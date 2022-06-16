#ifndef _QM_UTILS_HASHTABLE_H
#define _QM_UTILS_HASHTABLE_H


#if defined(__cplusplus) 
extern "C" {
#endif

typedef struct hash_item
{
 void *key;
 void *val;
 int size_val;
 struct hash_item *next;

}ht_item_t;

typedef struct {
    int             cnt;
    ht_item_t       *item;
} ht_t;

void *ht_init(int max_count);
int ht_add(void *ht, const void *key, unsigned int len_key, const void *val, unsigned int size_val);
void *ht_find(void *ht, const void *key, unsigned int key_len, void *val, int *size_val);
int ht_del(void *ht, const void *key, unsigned int len_key);
int ht_clear(void *ht);

#if defined(__cplusplus)
}
#endif


#endif

