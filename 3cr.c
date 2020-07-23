#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <signal.h>
#include "473_mm.h"
int v_frame,vm_size_g,n_frames_g,page_size_g,policy_g;
int p_frame=0;
int hex;
void *base_addr;


struct p_node
{void *addr;
int pno;
int frame_no;
int write_back;
int ref;
int chance;
struct p_node *next;
}*head=NULL,*tail=NULL;

void push_p(struct p_node **h,void* addr,int i,int j)
{	struct p_node *ptr=malloc(sizeof(struct p_node));
	struct p_node *temp=*h;
	ptr->addr=addr;
	ptr->pno=j;
	ptr->frame_no=p_frame;
	ptr->next=*h;
	ptr->write_back=0;
	ptr->ref=1;
	ptr->chance=1;
	if(*h!=NULL)
	{while(temp->next!=*h)
		temp=temp->next;
		temp->next=ptr;
	}
	else
	{ptr->next=ptr;
	*h=ptr;
	}	
	
}

int checkpgno(int i,void *add)
{	struct p_node *temp=head;
	while(temp->next!=head)
	{if(temp->pno==i)
		{	if(temp->ref==1)
			{mprotect(add,page_size_g,PROT_WRITE);
			mm_logger(i,1,-1,0,page_size_g*temp->frame_no+hex);
			temp->write_back=1;
			temp->ref=1;
			temp->chance=2;
			return 1;
			}
			else
			{
			temp->ref=1;
			mm_logger(i,2,-1,0,page_size_g*temp->frame_no+hex);
			if(temp->write_back==1)
			{temp->chance=2;
			mprotect(add,page_size_g,PROT_WRITE);
			}			
			else
			{mprotect(add,page_size_g,PROT_READ);
			temp->chance=1;
			}			
			return 1;
			}
		}
	temp=temp->next;
	}
	if(temp->pno==i)
	{
			if(temp->ref==1)
			{mprotect(add,page_size_g,PROT_WRITE);
			mm_logger(i,1,-1,0,page_size_g*temp->frame_no+hex);
			temp->write_back=1;
			temp->ref=1;
			temp->chance=2;
			return 1;
			}
			else
			{
			temp->ref=1;
			mm_logger(i,2,-1,0,page_size_g*temp->frame_no+hex);
			if(temp->write_back==1)
			{temp->chance=2;
			mprotect(add,page_size_g,PROT_WRITE);
			}			
			else
			{mprotect(add,page_size_g,PROT_READ);
			temp->chance=1;
			}			
			return 1;
			}
	}
	else
	return 0;
}
void evict (void *add, int pno1, struct p_node **h)
{	struct p_node *temp=*h;
	//h=h->next;
	while(1)
	{	if((temp->ref==0&&temp->write_back==0)||(temp->ref==0&&temp->chance==0))
		{//printf("1.ref %d chance %d pno %d \n",temp->ref,temp->chance,temp->pno);
		mprotect(temp->addr,page_size_g,PROT_NONE);
		if(temp->write_back==1)
		mm_logger(pno1,0,temp->pno,1,page_size_g*temp->frame_no+hex);
		else
		mm_logger(pno1,0,temp->pno,0,page_size_g*temp->frame_no+hex);	 	
		
	 	temp->addr=add;
	 	temp->pno=pno1;
	 	temp->write_back=0;
	 	temp->ref=1;
	 	temp->chance=1;
	 	mprotect(temp->addr,page_size_g,PROT_READ);
		temp=temp->next;
		*h=temp;
		//printf("header in %d\n",temp->pno);	
	 	return 0;
		}	
		if(temp->ref==1&&temp->write_back==0)
		{temp->ref=0;
		mprotect(temp->addr,page_size_g,PROT_NONE);
		temp->chance--;
		//printf("2.ref %d chance %d pno %d \n",temp->ref,temp->chance,temp->pno);
		temp=temp->next;
		continue;
		
		}
		if(temp->ref==0&&temp->write_back==1)
		{
		temp->chance--;
		//printf("3.ref %d chance %d pno %d \n",temp->ref,temp->chance,temp->pno);
		temp=temp->next;
		continue;
		}		
		if(temp->ref==1&&temp->write_back==1)
		{temp->ref=0;
		mprotect(temp->addr,page_size_g,PROT_NONE);
		temp->chance--;
		//printf("4.ref %d chance %d pno %d \n",temp->ref,temp->chance,temp->pno);
		temp=temp->next;
		continue;
		}
		//printf("danger");
	}
}



static void hdl (int sig, siginfo_t *siginfo, void *context)
{	int v_pageno,offset;
	void *add;
	int present;
	add=siginfo->si_addr;
	
	hex=(siginfo->si_addr-base_addr)&0xFFF;
	v_pageno=(siginfo->si_addr-base_addr)/page_size_g;
	add=base_addr+page_size_g*v_pageno;
	//printf ("Sending addr: %p %p %d %x\n",siginfo->si_addr,add,v_pageno,hex);
	
	if(head==NULL)
	{//printf("here \n");
	push_p(&head,add,p_frame,v_pageno);
	mprotect(add,page_size_g,PROT_READ);
	//printf(" frame %d \n", 1000*p_frame+hex);
	mm_logger(v_pageno,0,-1,0,page_size_g*p_frame+hex);
	p_frame++;
	}
	else
	{present = checkpgno(v_pageno,add);
	
	if(!present)
	{	if(p_frame<n_frames_g)
		{//printf("v_pageno = %d\n",v_pageno);
		push_p(&head,add,p_frame,v_pageno);
		mprotect(add,page_size_g,PROT_READ);
		//printf(" frame %d \n", page_size_g*p_frame+hex);
		mm_logger(v_pageno,0,-1,0,page_size_g*p_frame+hex);
		p_frame++;
		}
		else
		{
		evict(add,v_pageno,&head);				
		}
	}
	}
}	
	
void mm_init(void* vm, int vm_size, int n_frames, int page_size, int policy)
{	
	base_addr=vm;
	vm_size_g=vm_size;
	n_frames_g=n_frames;
	page_size_g=page_size;
	policy_g=policy;
	v_frame=vm_size/page_size;
	//printf("all inputs = vm %p vm_side %d n_frames %d page_size %d pg_size %d\n",vm,vm_size,n_frames,page_size,v_frame);
	mprotect(vm,vm_size,PROT_NONE);
		//handle_error("mprotect");
	//struct node *head=NULL,*tail=NULL;
	//for(int i=1;i<=v_frame;i++)
	//push_v(&v_head,vm+(page_size*(i-1)),i);
	struct sigaction act;
 	memset (&act, '\0', sizeof(act));
 
	/* Use the sa_sigaction field because the handles has two additional parameters */
	act.sa_sigaction = &hdl;
 
	/* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
	act.sa_flags = SA_SIGINFO;
 
	sigaction(SIGSEGV, &act, NULL) ;
 
	
	



}
