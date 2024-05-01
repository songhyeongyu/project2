/**********************************************************************
 * Copyright (c) 2019-2024
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "list_head.h"

/**
 * The process which is currently running
 */
#include "process.h"
extern struct process *current;

/**
 * List head to hold the processes ready to run
 */
extern struct list_head readyqueue;

/**
 * Resources in the system.
 */
#include "resource.h"
extern struct resource resources[NR_RESOURCES];

/**
 * Monotonically increasing ticks. Do not modify it
 */
extern unsigned int ticks;

/**
 * Quiet mode. True if the program was started with -q option
 */
extern bool quiet;

/***********************************************************************
 * Default FCFS resource acquision function
 *
 * DESCRIPTION
 *   This is the default resource acquision function which is called back
 *   whenever the current process is to acquire resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static bool fcfs_acquire(int resource_id)
{
	struct resource *r = resources + resource_id;

	if (!r->owner)
	{
		/* This resource is not owned by any one. Take it! */
		r->owner = current;
		return true;
	}

	/* OK, this resource is taken by @r->owner. */

	/* Update the current process state */
	current->status = PROCESS_BLOCKED;

	/* And append current to waitqueue */
	list_add_tail(&current->list, &r->waitqueue);

	/**
	 * And return false to indicate the resource is not available.
	 * The scheduler framework will soon call schedule() function to
	 * schedule out current and to pick the next process to run.
	 */
	return false;
}

/***********************************************************************
 * Default FCFS resource release function
 *
 * DESCRIPTION
 *   This is the default resource release function which is called back
 *   whenever the current process is to release resource @resource_id.
 *   The current implementation serves the resource in the requesting order
 *   without considering the priority. See the comments in sched.h
 ***********************************************************************/
static void fcfs_release(int resource_id)
{
	struct resource *r = resources + resource_id;

	/* Ensure that the owner process is releasing the resource */
	assert(r->owner == current);

	/* Un-own this resource */
	r->owner = NULL;

	/* Let's wake up ONE waiter (if exists) that came first */
	if (!list_empty(&r->waitqueue))
	{
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);

		/**
		 * Ensure the waiter is in the wait status
		 */
		assert(waiter->status == PROCESS_BLOCKED);

		/**
		 * Take out the waiter from the waiting queue. Note we use
		 * list_del_init() over list_del() to maintain the list head tidy
		 * (otherwise, the framework will complain on the list head
		 * when the process exits).
		 */
		list_del_init(&waiter->list);

		/* Update the process status */
		waiter->status = PROCESS_READY;

		/**
		 * Put the waiter process into ready queue. The framework will
		 * do the rest.
		 */
		list_add_tail(&waiter->list, &readyqueue);
	}
}

#include "sched.h"

/***********************************************************************
 * FCFS scheduler
 ***********************************************************************/
static int fcfs_initialize(void)
{
	return 0;
}

static void fcfs_finalize(void)
{
}

static struct process *fcfs_schedule(void)
{
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, the current process can be blocked
	 * while acquiring a resource. In this case just pick the next as well.
	 */
	if (!current || current->status == PROCESS_BLOCKED)
	{
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan)
	{
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue))
	{
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);

		/**
		 * Detach the process from the ready queue. Note that we use
		 * list_del_init() over list_del() to maintain the list head tidy.
		 * Otherwise, the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the process to run next */
	return next;
}

struct scheduler fcfs_scheduler = {
	.name = "FCFS",
	.acquire = fcfs_acquire,
	.release = fcfs_release,
	.initialize = fcfs_initialize,
	.finalize = fcfs_finalize,
	.schedule = fcfs_schedule,
};

/***********************************************************************
 * SJF scheduler
 ***********************************************************************/
static struct process *sjf_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
	struct process *next = NULL;
	struct process *compare = NULL;
	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();

	/**
	 * When there was no process to run in the previous tick (so does
	 * in the very beginning of the simulation), there will be
	 * no @current process. In this case, pick the next without examining
	 * the current process. Also, the current process can be blocked
	 * while acquiring a resource. In this case just pick the next as well.
	 */
	if (!current || current->status == PROCESS_BLOCKED)
	{
		goto pick_next;
	}

	/* The current process has remaining lifetime. Schedule it again */
	if (current->age < current->lifespan)
	{
		return current;
	}

pick_next:
	/* Let's pick a new process to run next */

	if (!list_empty(&readyqueue))
	{
		/**
		 * If the ready queue is not empty, pick the first process
		 * in the ready queue
		 */
		next = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry(compare, &readyqueue, list)
		{
			if (compare->lifespan < next->lifespan)
			{
				next = compare;
			}
		}
		/**
		 * Detach the process from the ready queue. Note that we use
		 * list_del_init() over list_del() to maintain the list head tidy.
		 * Otherwise, the framework will complain (assert) on process exit.
		 */
		list_del_init(&next->list);
	}

	/* Return the process to run next */
	return next;
}

struct scheduler sjf_scheduler = {
	.name = "Shortest-Job First",
	.acquire = fcfs_acquire,  /* Use the default FCFS acquire() */
	.release = fcfs_release,  /* Use the default FCFS release() */
	.schedule = sjf_schedule, /* TODO: Assign your schedule function
							  to this function pointer to activate
							  SJF in the simulation system */
};

/***********************************************************************
 * STCF scheduler
 ***********************************************************************/

static struct process *stcf_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
	struct process *next = NULL;
	struct process *tmp = NULL;
	// preemptive하다, 나보다 더 짧은 순간이 오면 그애를 pick한다.
	if (!current || current->status == PROCESS_BLOCKED)
	{
		goto pick_next;
	}

	if (current->age < current->lifespan)
	{
		list_for_each_entry(tmp, &readyqueue, list)
		{
			if ((current->lifespan - current->age) > (tmp->lifespan - tmp->age))
			{
				next = tmp;
				next->status = PROCESS_READY;
				list_add(&current->list, &readyqueue);
				current->status = PROCESS_BLOCKED;
				current = next;
				list_del_init(&next->list);
				break;
			}
		}
		return current;
	}

pick_next:
	if (!list_empty(&readyqueue))
	{
		next = list_first_entry(&readyqueue, struct process, list);
		list_for_each_entry(tmp, &readyqueue, list)
		{
			if (tmp->lifespan < next->lifespan)
			{
				next = tmp;
			}
		}
		list_del_init(&next->list);
	}

	return next;
}
struct scheduler stcf_scheduler = {
	.name = "Shortest Time-to-Complete First",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */

	/* You need to check the newly created processes to implement STCF.
	 * Have a look at @forked() callback.
	 */
	/* Obviously, you should implement stcf_schedule() and attach it here */
	.schedule = stcf_schedule,
};

/***********************************************************************
 * Round-robin scheduler
 ***********************************************************************/
static struct process *rr_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
	struct process *next = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();
	// rr은 preemptive 하다, policy는 ,스케줄될때마다(실행될때마다) 다른 프로세스 다시 readyqueue에 넣기
	if (!current || current->status == PROCESS_BLOCKED)
	{
		goto pick_next;
	}

	if (current->age < current->lifespan)
	{ // 매번 schedule될때마다 readqueue 뒤쪽으로 옮긴다.
		list_move_tail(&current->list, &readyqueue);
		goto pick_next;
	}

pick_next:

	if (!list_empty(&readyqueue))
	{
		next = list_first_entry(&readyqueue, struct process, list);
		list_del_init(&next->list);
	}

	return next;
}
struct scheduler rr_scheduler = {
	.name = "Round-Robin",
	.acquire = fcfs_acquire, /* Use the default FCFS acquire() */
	.release = fcfs_release, /* Use the default FCFS release() */
							 /* Obviously, ... */
	.schedule = rr_schedule};

/***********************************************************************
 * Priority scheduler
 ***********************************************************************/
static bool prio_acquire(int resource_id) // process가 resource를 차지하겠다 내놔라!!!ㄴ
{
	struct resource *r = resources + resource_id; // reource_id는 1~16까지 아무거나
	if (!r->owner)
	{
		// current가 현재 resource의 owner이다.
		r->owner = current;
		return true;
	}
	// resource가 필요로 하는 process에게 갔다.
	// blocked상태는 waitqueue로 들어가는 과정이다. 현재 process가 blocked상태로 들어가고
	current->status = PROCESS_BLOCKED;
	// currentprocess를 waitqueue에 넣어 놓는다.
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}

static void prio_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *tmp = NULL;
	assert(r->owner == current);
	r->owner = NULL;
	if (!list_empty(&r->waitqueue))
	{
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);
		list_for_each_entry(tmp, &r->waitqueue, list)
		{
			if (tmp->prio > waiter->prio)
			{
				waiter = tmp;
			}
		}

		// process의 resource가 있으면 waitqueue에 들어가서 기다려야 된다?
		assert(waiter->status == PROCESS_BLOCKED);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}
static struct process *prio_schedule(void)
{
	struct process *next = NULL;
	struct process *tmp = NULL;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();
	// prio는 preemptive prio가 높을 수록 먼저시행, resource를 먼저 풀어줘야된다.
	// pid를 통해서 지금 실행중인것을 control해야되는데 . 왜안돼
	if (!current || current->status == PROCESS_BLOCKED)
	{
		goto pick_next;
	}

	if (current->age < current->lifespan)
	{
		list_for_each_entry(tmp, &readyqueue, list)
		{
			if (current->pid != tmp->pid)
			{
				list_add(&current->list, &readyqueue);
				goto pick_next;
			}
			else
			{
				list_del_init(&current->list);
			}
		}
		return current;
	}

pick_next:
	// readyqueue에서 scheduleing을 해준 후 process를 선택한다.
	if (!list_empty(&readyqueue))
	{
		next = list_first_entry(&readyqueue, struct process, list);

		list_for_each_entry(tmp, &readyqueue, list)
		{

			if (tmp->prio > next->prio)
			{
				next = tmp;
			}
		}
		list_del_init(&next->list);
	}

	return next;
}

struct scheduler prio_scheduler = {
	.name = "Priority",
	/**
	 * Implement your own acqure/release function to make the priority
	 * scheduler correct.
	 */
	.acquire = prio_acquire,
	.release = prio_release,
	/* Implement your own prio_schedule() and attach it here */
	.schedule = prio_schedule,
};

/***********************************************************************
 * Priority scheduler with aging
 ***********************************************************************/
static bool pa_acquire(int resource_id) // process가 resource를 차지하겠다 내놔라!!!ㄴ
{
	struct resource *r = resources + resource_id; // reource_id는 1~16까지 아무거나
	if (!r->owner)
	{
		// current가 현재 resource의 owner이다.
		r->owner = current;
		return true;
	}
	// resource가 필요로 하는 process에게 갔다.
	// blocked상태는 waitqueue로 들어가는 과정이다. 현재 process가 blocked상태로 들어가고
	current->status = PROCESS_BLOCKED;
	// currentprocess를 waitqueue에 넣어 놓는다.
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}

static void pa_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *tmp = NULL;
	assert(r->owner == current);
	r->owner = NULL;
	if (!list_empty(&r->waitqueue))
	{
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);
		list_for_each_entry(tmp, &r->waitqueue, list)
		{
			if (tmp->prio > waiter->prio)
			{
				waiter = tmp;
			}
		}
		// process의 resource가 있으면 waitqueue에 들어가서 기다려야 된다?
		assert(waiter->status == PROCESS_BLOCKED);
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		list_add_tail(&waiter->list, &readyqueue);
	}
}

static struct process *pa_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
	struct process *next = NULL;
	struct process *tmp = NULL;
	// int a = 0;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();
	// pa는 preemptive 하다, policy는 rr기반 스케줄될때마다(실행될때마다) 다른 프로세스 다시 readyqueue에 넣기

	if (!current || current->status == PROCESS_BLOCKED) //current process가 없거나, current status가 blocked이면 다른걸 집어라
	{
		
		goto pick_next;
	}
	//picK_next가 끝나면
	//현재 prio를 +1을 시켜주면서 process가 current가 될때 
	if(current){
		//readyqueue에는 current process를 제외하고 다른 Process들이 존재한다. 따라서 tmp를 올려주면 current를 제외한 process들의 prio 증가
		list_for_each_entry(tmp,&readyqueue,list){
			// printf("be +1:%d %d\n",tmp->pid,tmp->prio);
			tmp->prio++;
			
			//readyqueue에는 current process가 존재하지않는다.
		
		}
		// printf("current:%d\n",current->pid);
	}

	if (current->age < current->lifespan) // current가 끝날때 까지 돌아라
	{	
		list_move_tail(&current->list, &readyqueue); // 현재거를 뒤로 옮기고 -> rr방식
		goto pick_next; // 다음거를 집어라
	}

pick_next:

	if (!list_empty(&readyqueue))
	{
		next = list_first_entry(&readyqueue,struct process,list);
		// printf("nextpid prio: %d %d\n",next->pid,next->prio);
		list_for_each_entry(tmp, &readyqueue, list)
		{	
			// printf("af +1 tmppid prio: %d %d\n",tmp->pid,tmp->prio);
			if (tmp->prio > next->prio) 
			// prio가 높은게 뽑히게 해라 같은 prio가 나오면-> ex) 20 20 next = 먼저reayqueue에 존재하는게 나온다
			//rr정책을 사용해서 최근에 사용된 process는 readyqueue마지막에 붙어있다.
			{	
				// printf("tmp pid prio: %d %d\n",tmp->pid,tmp->prio);
				next = tmp;
			}
		}
		next->prio = next->prio_orig; //next로 뽑힌 prio를 prio_orig로 만들어준다.
		list_del_init(&next->list);
	}

	return next;
}
struct scheduler pa_scheduler = {
	.name = "Priority + aging",
	/**
	 * Ditto
	 */
	.acquire = pa_acquire,
	.release = pa_release,
	.schedule = pa_schedule,
};

/***********************************************************************
 * Priority scheduler with priority ceiling protocol
 ***********************************************************************/
static bool pcp_acquire(int resource_id) // process가 resource를 차지하겠다 내놔라!!!ㄴ
{
	struct resource *r = resources + resource_id; // reource_id는 1~16까지 아무거나
	if (!r->owner)
	{
		r->owner = current;
		r->owner->prio = MAX_PRIO;
		// printf("r owner pid: %d\n",r->owner->pid);
		return true;
	}

	

	current->status = PROCESS_BLOCKED;
	list_add_tail(&current->list, &r->waitqueue);
	return false;
}

static void pcp_release(int resource_id)
{
	struct resource *r = resources + resource_id;
	struct process *tmp = NULL;
	assert(r->owner == current);
	
	current->prio = current->prio_orig;
	// printf("currentpid prio: %d %d\n",current->pid,current->prio);
	// printf("rowner pid prio: %d %d\n",r->owner->pid,current->prio);
	r->owner = NULL;

	if (!list_empty(&r->waitqueue))
	{
		struct process *waiter = list_first_entry(&r->waitqueue, struct process, list);
		list_for_each_entry(tmp, &r->waitqueue, list)
		{
			if (tmp->prio > waiter->prio)
			{
				waiter = tmp;
			}
		}
		// process의 resource가 있으면 waitqueue에 들어가서 기다려야 된다?
		assert(waiter->status == PROCESS_BLOCKED);
		// printf("realase rowner prio:%d\n",r->owner->prio);
		
		list_del_init(&waiter->list);
		waiter->status = PROCESS_READY;
		
		list_add_tail(&waiter->list, &readyqueue);
	}
}

static struct process *pcp_schedule(void)
{
	/**
	 * Implement your own SJF scheduler here.
	 */
	struct process *next = NULL;
	struct process *tmp = NULL;
	// int a = 0;

	/* You may inspect the situation by calling dump_status() at any time */
	// dump_status();
	// pcp는 preemptive 하다, policy는 rr기반 + prio를 max_prio까지 올려준다. 스케줄될때마다(실행될때마다) 다른 프로세스 다시 readyqueue에 넣기 -> 꼬리에 넣는다.

	if (!current || current->status == PROCESS_BLOCKED) //current process가 없거나, current status가 blocked이면 다른걸 집어라
	{
		goto pick_next;
	}
	// process 1이 acquire을 잡아야 되는데? 왜 3이 먼저잡아버리지?

	if (current->age < current->lifespan) // current가 끝날때 까지 돌아라
	{	
			list_move_tail(&current->list,&readyqueue);
			goto pick_next;
	}

pick_next:

	if (!list_empty(&readyqueue))
	{
		next = list_first_entry(&readyqueue,struct process,list);
		list_for_each_entry(tmp,&readyqueue,list){
			if(tmp->prio > next->prio)
			{
				next = tmp;
			}
			// printf("tmp :%d\n",tmp->prio);
		}
		list_del_init(&next->list);
	}

	return next;
}
struct scheduler pcp_scheduler = {
	.name = "Priority + PCP Protocol",
	/**
	 * Ditto
	 */
	.acquire = pcp_acquire,
	.release = pcp_release,
	.schedule = pcp_schedule,
};

/***********************************************************************
 * Priority scheduler with priority inheritance protocol
 ***********************************************************************/
struct scheduler pip_scheduler = {
	.name = "Priority + PIP Protocol",
	/**
	 * Ditto
	 */
};
