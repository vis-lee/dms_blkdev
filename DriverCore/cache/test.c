#include <linux/kthread.h>
#include <linux/signal.h>

#include "cache.h"

static struct Cache_Operations c_ops;
#define COUNT_PRIMES	1000
static int primes[COUNT_PRIMES];

#define COUNT_THREAD	20
static struct task_struct *threads[COUNT_THREAD];

static void init_primes(void)
{
	int i, c, p;

	primes[0] = 2;
	
	for (i = 1; i < COUNT_PRIMES; ++i) {
		p = primes[i-1]+1;
		while (1) {
			for (c = 0; c < i; ++c) {
				if (p%primes[c] == 0)
					break;
			}
			if (c == i) {
				primes[i] = p;
				break;
			} else
				++p;
		}
	}
}

static unsigned int random32(void)
{
	static unsigned int s1_n = 0;
	int p;
	++s1_n;
	if (s1_n >= 1000)
		s1_n = 0;
	p = primes[s1_n];
	return (((p & 4294967294) <<12) ^ (((p <<13) ^ p) >>19));
}

static unsigned long long random64(void)
{
	static unsigned int s1_n = 0;
	int p;
	unsigned long long ret;

	++s1_n;
	if (s1_n >= 1000)
		s1_n = 0;
	p = primes[s1_n];
	ret = p;
	ret = ret << 32;
	return ret | (((p & 4294967294) <<12) ^ (((p <<13) ^ p) >>19));
}


void test(void)
{
	struct meta_data ***meta_data;
	unsigned long long i;
	unsigned int count;
	unsigned long long start;
	unsigned int j, c;


	for (c = 0; c < 10000; ++c) {

	count = random32();
	start = random64();

	count = count % 100;
	if (count < 10)
		count = 10;
	
	meta_data = kmalloc(count*sizeof(struct meta_data **), GFP_KERNEL);

	//ccma_printk("start = %llx, count = %u\n", start, count);

	/* [0, COUNT-1] */
	for (i = 0; i < count; ++i)
		meta_data[i] = (struct meta_data **)(start+i);

	BUG_ON(c_ops.Update_Metadata_Cache_Item(1,
				start, count,
				(struct meta_data **)meta_data));
	j = c_ops.Get_Metadata_Cache_Item(1,
				start, count,
				meta_data);
	if (j != count)
		ccma_printk("count %d gm %d\n", count, j);

	/* [0, COUNT-1] */
	if (j)
	for (i = 0; i < count; ++i) {
		if (meta_data[i]) {
			if (*meta_data[i] != (start + i))
				ccma_printk("%lld => %lld\n",
						(start + i),
						*meta_data[i]);
		}
	}
	
	if (j)
	BUG_ON(c_ops.Put_Metadata_Cache_Item(1,
				start, count,
				(struct meta_data ***)meta_data));
	kfree(meta_data);
	} // end of big loop

}

void init_test(void)
{
	int i;

	init_primes();

	Init_Cache_Module(&c_ops);

	// volume id = 1, size = 1
	ccma_printk("cv %d\n", c_ops.Create_Volume_Cache(1, 10000));

	for (i = 0; i < COUNT_THREAD; ++i)
		threads[i] = kthread_run((void *)test, NULL, "ccma_test");
}

void exit_test(void)
{
	int i;
	for (i = 0; i < COUNT_THREAD; ++i)
		if (threads[i])
			kill_proc(threads[i]->pid, SIGKILL, 1);
	if (c_ops.Release_Volume_Cache)
		ccma_printk("rv %d\n", c_ops.Release_Volume_Cache(1));
}
