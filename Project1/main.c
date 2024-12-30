#include "bitmap.h"
#include "list.h"
#include "round.h"
#include "debug.h"
#include "hash.h"
#include "hex_dump.h"
#include "limits.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

bool less_func_list(const struct list_elem *a, const struct list_elem *b, void *aux) // Compares two list elements based on their data values.
{
    struct list_item *item_a = list_entry(a, struct list_item, elem);
    struct list_item *item_b = list_entry(b, struct list_item, elem);
    
    return item_a->data < item_b->data;
}
bool less_func_hash(const struct hash_elem *a, const struct hash_elem *b, void *aux) // Compares two hash elements based on their data values.
{
    return a->data < b->data;
}
unsigned hash_func (const struct hash_elem *e, void *aux) // Generates a hash value from a hash element's data.
{ 
    return hash_int(e->data);
}
void hash_destructor (struct hash_elem *e, void *aux) // Frees memory allocated for a hash element.
{
    free(e);
}
void hash_square(struct hash_elem *e, void *aux) // Squares the data of a hash element.
{
    e->data *= e->data;
}
void hash_triple(struct hash_elem *e, void *aux) // Cubes the data of a hash element.
{
    e->data = e->data * e->data * e->data;
}

int main(int argc, char *argv[])
{

    char str[50];
    char todo[30], type[10] , name[10],t_f[6], apply[8];
    int start, end, cnt, size_bmap, input_num=0, idx;
    int todo_idx;
    int i,j;
    struct list *mylists[10];
    struct hash *myhashes[10];
    struct bitmap *mybmaps[10];
    struct hash_iterator h_i;
   
    while(1)
    {
        fgets(str,50,stdin); // Reads up to 49 characters from standard input into str
        if(!strncmp(str,"quit",4)) break; // Exits the loop
        
        str[strlen(str)-1] = '\0';

        if(strncmp(str,"create",6) == 0) // creates 'list' or 'hashtable' or 'bitmap'
        {
            sscanf(str,"%s %s %s",todo,type,name);
            if(strncmp(type,"list",4) == 0) // create 'list' 
            {
                struct list *mylist = (struct list*)malloc(sizeof(struct list));
                list_init(mylist); 
                idx = name[4]-'0'; // find the index of data struct
                mylists[idx] = mylist; 
            }
            else if(strncmp(type,"bitmap",6) == 0) // create 'bitmap'
            {
                sscanf(str,"%s %s %s %d",todo, type, name, &size_bmap);
                struct bitmap *mybmap = bitmap_create(size_bmap);
                idx = name[2]-'0'; // find the index of data struct
                mybmaps[idx] = mybmap;
            }
            else if(strncmp(type,"hashtable",9) == 0) // create 'hashtable'
            {
                struct hash *myhash = (struct hash*)malloc(sizeof(struct hash));
                hash_init(myhash,hash_func,less_func_hash,NULL);
                idx = name[4]-'0'; // find the index of data struct
                myhashes[idx] = myhash;
            }
        }
        else if(strncmp(str,"dumpdata",8) == 0) // prints contents of each data structs
        {
            sscanf(str,"%s %s",todo, name);
            if(strncmp(name,"list",4) == 0) // prints contents of 'list'
            {
                idx = name[4]-'0';
                struct list_elem *elem_ptr = list_begin(mylists[idx]);
                for(i=0;i<list_size(mylists[idx]);i++) 
                {
                    struct list_item *item_ptr = list_entry(elem_ptr, struct list_item,elem);
                    printf("%d ",item_ptr->data);
                    elem_ptr = list_next(elem_ptr);
                }
                printf("\n");
            }
            else if(strncmp(name,"hash",4) == 0) // prints contents of 'hashtable'
            {
                idx = name[4]-'0';
                struct hash_elem *elem_ptr = (struct hash_elem*)malloc(sizeof(struct hash_elem));
                hash_first(&h_i,myhashes[idx]);
                while(hash_next(&h_i))
                {
                    printf("%d ",hash_cur(&h_i)->data);
                }
                printf("\n");
                
            }
            else if(strncmp(name,"bm",2) == 0) // prints contents of 'bitmap'
            {
                idx = name[2]-'0';
                for(i=0; i<bitmap_size(mybmaps[idx]);i++)
                {
                    if(bitmap_test(mybmaps[idx],i)) // bit == 0
                    {
                        printf("1");
                    }
                    else printf("0");
                }
                printf("\n");
            }
        }
        else if(!strncmp(str,"delete",6)) // deletes each data structs
        {
            sscanf(str,"%s %s",todo, name);
            if(!strncmp(name,"list",4))
            {
                idx = name[4]-'0'; //index
                while(!list_empty(mylists[idx])) 
                {
                    struct list_elem *elem_ptr = list_pop_front(mylists[idx]);
                    elem_ptr = list_remove(elem_ptr);
                }
            }
            else if(strncmp(name,"hashtable",9) == 0)
            {
                idx = name[4]-'0';
                hash_destroy(myhashes[idx],hash_destructor);
            }
            else if(strncmp(name,"bm",2) == 0)
            {
                idx = name[2]-'0';
                bitmap_destroy(mybmaps[idx]);
            }
        }
        
        else if(strncmp(str,"list",4) == 0) // list functions
        {
            sscanf(str,"%s",todo);
            if(strncmp(todo,"list_push_back",14) == 0) // list_push_back
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[4]-'0';
                struct list_item *item_ptr = (struct list_item *)malloc(sizeof(struct list_item));
                item_ptr->data = input_num;
                list_push_back(mylists[idx],&item_ptr->elem);
            }
            else if(!strcmp(todo,"list_push_front")) // list_push_front
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[4]-'0';
                struct list_item *item_ptr = (struct list_item *)malloc(sizeof(struct list_item));
                item_ptr->data = input_num;
                list_push_front(mylists[idx],&item_ptr->elem);
            }
            else if(!strncmp(todo,"list_front",10)) // list_front
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4]-'0';
                struct list_elem *elem_ptr = (struct list_elem *)malloc(sizeof(struct list_elem));
                struct list_item *item_ptr = (struct list_item *)malloc(sizeof(struct list_item));
                elem_ptr = list_front(mylists[idx]);
                item_ptr = list_entry(elem_ptr, struct list_item,elem);
                printf("%d\n",item_ptr->data);
            }
            else if(!strncmp(todo,"list_back",9)) // list_back 
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4]-'0';
                struct list_elem *elem_ptr = (struct list_elem *)malloc(sizeof(struct list_elem));
                struct list_item *item_ptr = (struct list_item *)malloc(sizeof(struct list_item));
                elem_ptr = list_back(mylists[idx]);
                item_ptr = list_entry(elem_ptr, struct list_item,elem);
                printf("%d\n",item_ptr->data);

            }
            else if(!strncmp(todo,"list_pop_back",14)) // list_pop_back
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4]-'0';
                struct list_elem *elem_ptr = (struct list_elem *)malloc(sizeof(struct list_elem));
                elem_ptr = list_pop_back(mylists[idx]);
            }
            else if(!strncmp(todo,"list_pop_front",14)) // list_pop_front 
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4]-'0';
                struct list_elem *elem_ptr = (struct list_elem *)malloc(sizeof(struct list_elem));
                elem_ptr = list_pop_front(mylists[idx]);
            }
            else if(!strncmp(todo,"list_insert_ordered",19)) // list_insert_ordered
            { 
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[4]-'0';
                struct list_item *item_ptr = (struct list_item *)malloc(sizeof(struct list_item));
                item_ptr->data = input_num;
                list_insert_ordered(mylists[idx],&item_ptr->elem, less_func_list,NULL);
            }
            else if(!strcmp(todo,"list_insert")) // list_insert
            {
                sscanf(str,"%s %s %d %d",todo,name,&todo_idx,&input_num );
                idx = name[4]-'0';
                
                struct list_item *item_ptr = (struct list_item *)malloc(sizeof(struct list_item));
                item_ptr->data = input_num;
    
                struct list_elem *insert_ptr = list_begin(mylists[idx]);
                for(i=0;i<todo_idx;i++)
                    insert_ptr = list_next(insert_ptr);
                
                list_insert(insert_ptr,&item_ptr->elem);
            }
            else if(!strcmp(todo,"list_empty")) // list_empty
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4]-'0';
                if(list_empty(mylists[idx]))
                {
                    printf("true\n");
                }
                else printf("false\n");
            }
            else if(!strcmp(todo,"list_size")) // list_size
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4]-'0';
                printf("%zu\n",list_size(mylists[idx]));
            }
            else if(!strcmp(todo,"list_max")) // list_max 
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4]-'0';
                struct list_elem *elem_ptr = list_max(mylists[idx],less_func_list,NULL);
                struct list_item *item_ptr = list_entry(elem_ptr,struct list_item,elem);
                printf("%d\n",item_ptr->data);
            }
            else if(!strcmp(todo,"list_min")) // list_min 
            {
                
                sscanf(str,"%s %s",todo,name);
                idx = name[4]-'0';
                struct list_elem *elem_ptr = list_min(mylists[idx],less_func_list,NULL);
                struct list_item *item_ptr = list_entry(elem_ptr,struct list_item,elem);
                printf("%d\n",item_ptr->data);
            }
            else if(!strcmp(todo,"list_remove")) // list_remove
            {
                sscanf(str,"%s %s %d",todo ,name, &todo_idx);
                idx = name[4]-'0';

                struct list_elem *elem_ptr = list_front(mylists[idx]);
                for(i=0;i<todo_idx;i++)
                    elem_ptr = list_next(elem_ptr);
                
                list_remove(elem_ptr);
            }
            else if(!strcmp(todo,"list_reverse")) // list_reverse 
            {
                sscanf(str,"%s %s",todo ,name);
                idx = name[4]-'0';
                list_reverse(mylists[idx]);
            }
            else if(!strcmp(todo,"list_shuffle")) // list_shuffle 
            {
                sscanf(str,"%s %s",todo ,name);
                idx = name[4]-'0';
                list_shuffle(mylists[idx]);
            }  
            else if(!strcmp(todo,"list_sort")) // list_sort 
            {
                sscanf(str,"%s %s",todo ,name);
                idx = name[4]-'0';
                list_sort(mylists[idx],less_func_list,NULL);
            }  
            else if(!strcmp(todo,"list_splice")) // list_splice
            {
                int before,front,back,idx1,idx2;
                char list1[6],list2[6];
                sscanf(str,"%s %s %d %s %d %d",todo ,list1,&before,list2,&front,&back);
                idx1 = list1[4]-'0';
                idx2 = list2[4]-'0';
                
                struct list_elem *elem_ptr1 = list_front(mylists[idx1]);
                for(i=0;i<before;i++)
                    elem_ptr1 = list_next(elem_ptr1);
                
                
                struct list_elem *elem_ptr2 = list_front(mylists[idx2]);
                for(i=0;i<front;i++)
                    elem_ptr2 = list_next(elem_ptr2);

                struct list_elem *elem_ptr3 = list_front(mylists[idx2]);
                for(i=0;i<back;i++)
                    elem_ptr3 = list_next(elem_ptr3);

                list_splice(elem_ptr1,elem_ptr2,elem_ptr3);
                
            }   
            else if(!strcmp(todo,"list_swap")) // list_swap
            {
                int idx3, idx4;
                sscanf(str,"%s %s %d %d",todo ,name,&idx3,&idx4);
                idx = name[4]-'0';
                struct list_elem *elem_ptr1 = list_begin(mylists[idx]);
                for(i=0;i<idx3;i++)
                    elem_ptr1 = list_next(elem_ptr1);
                
                struct list_elem *elem_ptr2 = list_begin(mylists[idx]);
                for(i=0;i<idx4;i++)
                    elem_ptr2 = list_next(elem_ptr2);

                list_swap(elem_ptr1,elem_ptr2);
            }      
            else if(!strcmp(todo,"list_unique")) // list_unique 
            {
                int idx1, idx2;
                char list1[6], list2[6];
                sscanf(str,"%s %s %s",todo ,list1,list2);
                idx1 = list1[4]-'0'; idx2 = list2[4]-'0';
                if(!strcmp(list2,list1))
                {
                    list_unique(mylists[idx1],NULL,less_func_list,NULL);
                }
                else
                {
                    list_unique(mylists[idx1],mylists[idx2],less_func_list,NULL);
                }
                
            }
        }
        else if(!strncmp(str,"hash",4)) // hash functions
        {
            sscanf(str,"%s",todo);
            if(!strcmp(todo,"hash_insert")) // hash_insert
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[4] - '0';
                struct hash_elem *elem_ptr = (struct hash_elem *)malloc(sizeof(struct hash_elem));
                elem_ptr->data = input_num;
                hash_insert(myhashes[idx],elem_ptr);
            }
            else if(!strcmp(todo,"hash_apply")) // hash_apply
            {
                sscanf(str,"%s %s %s",todo,name,apply);
                struct hash_elem *elem_ptr = (struct hash_elem*)malloc(sizeof(struct hash_elem));
                hash_first(&h_i,myhashes[idx]);
                if(!strcmp(apply,"square"))
                {
                    while(hash_next(&h_i))
                    {
                        elem_ptr = hash_cur(&h_i);
                        hash_square(elem_ptr,NULL);
                    }
                }
                else if(!strcmp(apply,"triple"))
                {
                    while(hash_next(&h_i))
                    {
                        elem_ptr = hash_cur(&h_i);
                        hash_triple(elem_ptr,NULL);
                    }
                }
            }
            else if(!strcmp(todo,"hash_delete")) // hash_delete
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                struct hash_elem *elem_ptr = (struct hash_elem*)malloc(sizeof(struct hash_elem));
                hash_first(&h_i,myhashes[idx]);
                while(hash_next(&h_i))
                {
                    elem_ptr = hash_cur(&h_i);
                    if(elem_ptr->data == input_num)
                    {
                        hash_delete(myhashes[idx],elem_ptr);
                        break;
                    }
                }
            }
            else if(!strcmp(todo,"hash_empty")) // hash_empty
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4] - '0';
                if(hash_empty(myhashes[idx]))
                    printf("true\n");
                else
                    printf("false\n");
            }
            else if(!strcmp(todo,"hash_size")) // hash_size
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4] - '0';
                printf("%zu\n",hash_size(myhashes[idx]));
            }
            else if(!strcmp(todo,"hash_clear")) // hash_clear
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[4] - '0';
                hash_clear(myhashes[idx],hash_destructor);
            }
            else if(!strcmp(todo,"hash_replace")) // hash_replace
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[4] - '0';
                struct hash_elem *elem_ptr = (struct hash_elem*)malloc(sizeof(struct hash_elem));
                elem_ptr->data = input_num;
                hash_replace(myhashes[idx],elem_ptr);
            }
            else if(!strcmp(todo,"hash_find")) // hash_find
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[4] - '0';
                struct hash_elem *elem_ptr = (struct hash_elem*)malloc(sizeof(struct hash_elem));
                elem_ptr->data = input_num;
                hash_first(&h_i,myhashes[idx]);
                while(hash_next(&h_i))
                {
                    // elem_ptr = hash_cur(&h_i);
                    if(hash_find(myhashes[idx],elem_ptr) != NULL)
                    {
                        printf("%d\n",elem_ptr->data);
                        break;
                    }
                }
            }
        }
        else if(!strncmp(str,"bitmap",6)) // bitmap functions
        {
            sscanf(str,"%s",todo);
            if(!strcmp(todo,"bitmap_mark")) // bitmap_mark
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[2] - '0';
                bitmap_mark(mybmaps[idx],input_num);                
            }
            else if(!strcmp(todo,"bitmap_all")) // bitmap_all
            {
                sscanf(str,"%s %s %d %d",todo,name,&start,&end);
                idx = name[2] - '0';
                if(bitmap_all(mybmaps[idx],start,end))
                {
                    printf("true\n");
                }
                else printf("false\n");
            }
            else if(!strcmp(todo,"bitmap_any")) // bitmap_any
            {
                sscanf(str,"%s %s %d %d",todo,name,&start,&cnt);
                idx = name[2] - '0';
                if(bitmap_any(mybmaps[idx],start,cnt))
                {
                    printf("true\n");
                }
                else printf("false\n");
            }
            else if(!strcmp(todo,"bitmap_contains")) // bitmap_contains
            {
                sscanf(str,"%s %s %d %d %s",todo,name,&start,&cnt,t_f);
                idx = name[2] - '0';
                bool val;
                
                if(!strcmp(t_f,"true")) val = true;
                else val = false;

                if(bitmap_contains(mybmaps[idx],start,cnt,val))
                {
                    printf("true\n");
                }
                else printf("false\n");
            }
            else if(!strcmp(todo,"bitmap_count")) // bitmap_count
            {
                sscanf(str,"%s %s %d %d %s",todo,name,&start,&cnt,t_f);
                idx = name[2] - '0';
                bool val;
                
                if(!strcmp(t_f,"true")) val = true;
                else val = false;

                printf("%zu\n",bitmap_count(mybmaps[idx],start,cnt,val));
            }
            else if(!strcmp(todo,"bitmap_dump")) // bitmap_dump
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[2] - '0';
                bitmap_dump(mybmaps[idx]);
            }
            else if(!strcmp(todo,"bitmap_expand")) // bitmap_expand
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[2] - '0';

                struct bitmap *mybmap = bitmap_expand(mybmaps[idx], input_num);
                if (mybmap == NULL)
                {
                    continue;
                }
                else
                {
                    bitmap_destroy(mybmaps[idx]);
                    mybmaps[idx] = mybmap;
                }
            }
            else if(!strcmp(todo,"bitmap_set_all")) // bitmap_set_all
            {
                sscanf(str,"%s %s %s",todo,name,t_f);
                idx = name[2] - '0';
                bool val;
                
                if(!strcmp(t_f,"true")) val = true;
                else val = false;

                bitmap_set_all(mybmaps[idx],val);
            }
            else if(!strcmp(todo,"bitmap_flip")) // bitmap_flip
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[2] - '0';
                bitmap_flip(mybmaps[idx],input_num);
            }
            else if(!strcmp(todo,"bitmap_none")) // bitmap_none
            {
                sscanf(str,"%s %s %d %d",todo,name,&start,&cnt);
                idx = name[2] - '0';

                if(bitmap_none(mybmaps[idx],start,cnt))
                {
                    printf("true\n");
                }
                else printf("false\n");
                
            }
            else if(!strcmp(todo,"bitmap_reset")) // bitmap_reset
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[2] - '0';
                bitmap_reset(mybmaps[idx],input_num);
            }
            else if(!strcmp(todo,"bitmap_scan_and_flip"))  // bitmap_scan_and_flip
            {
                sscanf(str,"%s %s %d %d %s",todo,name,&start,&cnt,t_f);
                idx = name[2] - '0';
                bool val;
                
                if(!strcmp(t_f,"true")) val = true;
                else val = false;

                printf("%zu\n",bitmap_scan_and_flip(mybmaps[idx],start,cnt,val));
            }
            else if(!strcmp(todo,"bitmap_scan")) // bitmap_scan
            {
                sscanf(str,"%s %s %d %d %s",todo,name,&start,&cnt,t_f);
                idx = name[2] - '0';
                bool val;
                
                if(!strcmp(t_f,"true")) val = true;
                else val = false;

                printf("%zu\n",bitmap_scan(mybmaps[idx],start,cnt,val));
            }
            else if(!strcmp(todo,"bitmap_set_multiple")) // bitmap_set_multiple
            {
                sscanf(str,"%s %s %d %d %s",todo,name,&start,&cnt,t_f);
                idx = name[2] - '0';
                bool val;
                
                if(!strcmp(t_f,"true")) val = true;
                else val = false;
                
                bitmap_set_multiple(mybmaps[idx],start,cnt,val);  
            }
            else if(!strcmp(todo,"bitmap_set")) // bitmap_set
            {
                sscanf(str,"%s %s %d %s",todo,name,&start,t_f);
                idx = name[2] - '0';
                bool val;
                
                if(!strcmp(t_f,"true")) val = true;
                else val = false;
                
                bitmap_set(mybmaps[idx],start,val);  
            }
            else if(!strcmp(todo,"bitmap_size")) // bitmap_size
            {
                sscanf(str,"%s %s",todo,name);
                idx = name[2] - '0';
                printf("%zu\n",bitmap_size(mybmaps[idx]));
            }
            else if(!strcmp(todo,"bitmap_test")) // bitmap_test
            {
                sscanf(str,"%s %s %d",todo,name,&input_num);
                idx = name[2] - '0';
                
                if(bitmap_test(mybmaps[idx],input_num))
                {
                    printf("true\n");
                }
                else printf("false\n");
            }
        }
    }

    
    return 0;
}