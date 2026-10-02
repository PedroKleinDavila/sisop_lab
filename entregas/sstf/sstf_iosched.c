/*
 * Shortest Seek Time First I/O scheduler for Linux 4.13.9.
 * Based on the course skeleton and block/noop-iosched.c.
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

struct sstf_data {
	struct list_head queue;
	sector_t head;
};

static void sstf_merged_requests(struct request_queue *q,
				 struct request *rq, struct request *next)
{
	list_del_init(&next->queuelist);
}

static sector_t distance(sector_t a, sector_t b)
{
	return a > b ? a - b : b - a;
}

static int sstf_dispatch(struct request_queue *q, int force)
{
	struct sstf_data *data = q->elevator->elevator_data;
	struct request *request, *chosen = NULL;
	sector_t best_distance = (sector_t)-1;
	sector_t start;

	list_for_each_entry(request, &data->queue, queuelist) {
		sector_t current = distance(data->head, blk_rq_pos(request));
		if (current < best_distance) {
			chosen = request;
			best_distance = current;
		}
	}
	if (!chosen)
		return 0;
	start = blk_rq_pos(chosen);
	list_del_init(&chosen->queuelist);
	data->head = start + blk_rq_sectors(chosen);
	pr_info("[SSTF] dsp %c %llu\n",
		rq_data_dir(chosen) ? 'W' : 'R', (unsigned long long)start);
	elv_dispatch_sort(q, chosen);
	return 1;
}

static void sstf_add_request(struct request_queue *q, struct request *rq)
{
	struct sstf_data *data = q->elevator->elevator_data;
	list_add_tail(&rq->queuelist, &data->queue);
	pr_info("[SSTF] add %c %llu\n",
		rq_data_dir(rq) ? 'W' : 'R',
		(unsigned long long)blk_rq_pos(rq));
}

static struct request *sstf_former_request(struct request_queue *q,
					   struct request *rq)
{
	struct sstf_data *data = q->elevator->elevator_data;
	if (rq->queuelist.prev == &data->queue)
		return NULL;
	return list_prev_entry(rq, queuelist);
}

static struct request *sstf_latter_request(struct request_queue *q,
					   struct request *rq)
{
	struct sstf_data *data = q->elevator->elevator_data;
	if (rq->queuelist.next == &data->queue)
		return NULL;
	return list_next_entry(rq, queuelist);
}

static int sstf_init_queue(struct request_queue *q, struct elevator_type *type)
{
	struct elevator_queue *elevator = elevator_alloc(q, type);
	struct sstf_data *data;

	if (!elevator)
		return -ENOMEM;
	data = kmalloc_node(sizeof(*data), GFP_KERNEL, q->node);
	if (!data) {
		kobject_put(&elevator->kobj);
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&data->queue);
	data->head = 0;
	elevator->elevator_data = data;
	spin_lock_irq(q->queue_lock);
	q->elevator = elevator;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void sstf_exit_queue(struct elevator_queue *elevator)
{
	struct sstf_data *data = elevator->elevator_data;
	BUG_ON(!list_empty(&data->queue));
	kfree(data);
}

static struct elevator_type elevator_sstf = {
	.ops.sq = {
		.elevator_merge_req_fn = sstf_merged_requests,
		.elevator_dispatch_fn = sstf_dispatch,
		.elevator_add_req_fn = sstf_add_request,
		.elevator_former_req_fn = sstf_former_request,
		.elevator_latter_req_fn = sstf_latter_request,
		.elevator_init_fn = sstf_init_queue,
		.elevator_exit_fn = sstf_exit_queue,
	},
	.elevator_name = "sstf",
	.elevator_owner = THIS_MODULE,
};

static int __init sstf_init(void)
{
	return elv_register(&elevator_sstf);
}

static void __exit sstf_exit(void)
{
	elv_unregister(&elevator_sstf);
}

module_init(sstf_init);
module_exit(sstf_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Shortest Seek Time First single-queue I/O scheduler");
