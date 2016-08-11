#ifndef _CLIST_H
#define _CLIST_H

typedef unsigned int    size_t; 		

#define container_of(ptr, type, member) ({      \
const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
(type *)( (char *)__mptr - offsetof(type,member) );})


#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/* struct clist definition. It is at the same time the type of the tail
 pointer of the circular list and the type of field used to link the elements */
/*
 
 struct clist {					//definito in types.h
    struct clist *next;		
};

*/


/* constant used to initialize an empty list */

#define CLIST_INIT {NULL}


/* add the structure pointed to by elem as the last element of a circular list */
/* clistp is the address of the tail pointer (struct clist *) */
/* member is the field of *elem used to link this list */

#define clist_enqueue(elem, clistp, member) \
    if (clist_empty(*(clistp))){    						/*caso lista vuota*/  \
        (clistp)->next = &(elem)->member;  \
        (elem)->member.next = &(elem->member); \
    } \
    else { (elem)->member.next = (clistp)->next->next;     /*caso lista non vuota*/ \
        (clistp)->next->next = &(elem)->member; \
        (clistp)->next = &(elem)->member; \
    }


/* add the structure pointed to by elem as the first element of a circular list */
/* clistp is the address of the tail pointer (struct clist *) */
/* member is the field of *elem used to link this list */

#define clist_push(elem, clistp, member) \
    if (clist_empty(*(clistp))){ 						/*caso lista vuota*/\
		(clistp)->next = &(elem)->member;  \
        (elem)->member.next = &(elem->member);  \
    }\
    else { 												/*caso lista non vuota*/\
		(elem)->member.next = (clistp)->next->next; \
		(clistp)->next->next= &(elem)->member;}



/* clist_empty returns true in the circular list is empty, false otherwise */
/* clistx is a struct clist */

#define clist_empty(clistx)   \
    (((clistx).next)==NULL)


/* return the pointer of the first element of the circular queue.
 elem is also an argument to retrieve the type of the element */
/* member is the field of *elem used to link this list */
 /*rivedere cosa delle graffe */
#define clist_head(elem, clistx, member)    \
    if (!clist_empty(*(clistx))){\
    	elem=container_of((clistx).next->next, typeof(*elem), member); \
    }\
    else NULL;
    



/* return the pointer of the last element of the circular queue.
 elem is also an argument to retrieve the type of the element */
/* member is the field of *elem used to link this list */

#define clist_tail(elem, clistx, member)    \
    if (!clist_empty(*(clistx))){\
    	elem=container_of((clistx).next, typeof(*elem), member); \
    }\
    else NULL;



/* clist_pop and clist__dequeue are synonyms */
/* delete the first element of the list (this macro does not return any value) */
/* clistp is the address of the tail pointer (struct clist *) */

#define clist_pop(clistp) clist__dequeue(clistp)
#define clist_dequeue(clistp) \
	if(!clist_empty(*(clistp))) \
    {if((clistp)->next==(clistp)->next->next) 		/*caso lista vuota o con un solo elemento*/\
        (clistp)->next=NULL; \
    else   											/*caso almeno due elementi*/ \
        (clistp)->next->next=(clistp)->next->next->next;\
    }\
    else NULL;


/* delete from a circular list the element whose pointer is elem */
/* clistp is the address of the tail pointer (struct clist *) */
/* member is the field of *elem used to link this list */

#define clist_delete(elem, clistp, member) ({ \
    typeof(elem) scan;\
    void *tmp=NULL;  \
    int x = 1;\
    clist_foreach(scan, clistp, member, tmp) { \
        if(&(elem)->member==&(scan)->member){ \
            clist_foreach_delete(scan, clistp, member, tmp);  \
            x = 0;						/*cancellazione è andata a buon fine*/\
            break; \
        }\
    }  x; \
})



/* this macro has been designed to be used as a for instruction,
 the instruction (or block) following clist_foreach will be repeated for each element
 of the circular list. elem will be assigned to each element */
/* clistp is the address of the tail pointer (struct clist *) */
/* member is the field of *elem used to link this list */
/* tmp is a void * temporary variable */

#define clist_foreach(scan, clistp, member, tmp)  \
if (!clist_empty(*(clistp))) \
for ((scan)=container_of((clistp)->next->next, typeof(*scan), member), (tmp)=NULL; /*inizializzazione*/\
     ((struct clist *)(tmp)) != (clistp)->next; 									/*controllo*/\
     (tmp) = &(scan)->member, (scan) = container_of(((scan)->member.next) , typeof(*scan),member)  ) /*incremento*/



/* this macro should be used after the end of a clist_foreach cycle
 using the same args. it returns false if the cycle terminated by a break,
 true if it scanned all the elements */

#define clist_foreach_all(scan, clistp, member, tmp) ({ \
    ((tmp) == (clistp)->next)  })


/* this macro should be used *inside* a clist_foreach loop to delete the
 current element */
 
#define clist_foreach_delete(scan, clistp, member, tmp) \
    if((clistp)->next->next==&(scan)->member){   /*caso testa*/ \
        clist_dequeue(clistp);} \
    else{ \
            if((clistp)->next==&(scan)->member){   /*caso coda*/ \
                ((struct clist *)(tmp))->next=(scan)->member.next;  \
                (clistp)->next=(tmp);   \
            }else {    							/*altri casi*/  \
            ((struct clist *)(tmp))->next=(scan)->member.next;}\
    }



/* this macro should be used *inside* a clist_foreach loop to add an element
	before the current one */
	
#define clist_foreach_add(elem, scan, clistp, member, tmp)    \
    if((tmp)!=NULL){   								/*caso in cui l'elemento non va inserito in testa*/\
        ((struct clist *)(tmp))->next=&(elem)->member;   \
        (elem)->member.next=&(scan)->member; \
    }   \
    else{ \
        clist_push(elem, clistp, member);  			/*caso testa*/\
    }

#endif
















