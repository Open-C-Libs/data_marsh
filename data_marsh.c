#include <string.h>

#include "data_marsh.h"
#include "../mem_alloc/mem_alloc.c"


#define RBTREE_BLACK   0
#define RBTREE_RED     1
#define RBTREE_STACK   256

#define CPD_CONSTRUCT(a, b) a##b

#define CPD_ENCODE_CONSTRUCT(type, name)                                    \
int32_t CPD_CONSTRUCT(dm_encode, name)(dm_encode *ctx, const type name) {   \
return dm_encode_bint(ctx, (uint64_t) name);                                \
}

#define CPD_DECODE_CONSTRUCT(type, name)                                    \
int32_t CPD_CONSTRUCT(dm_decode, name)(dm_decode *ctx, type *name) {        \
return dm_decode_bint(ctx, (uint64_t *) name);                              \
}

typedef enum {
    dm_basic_int = 0x00,
    dm_basic_var = 0x02,

    dm_basic_int64 = dm_basic_int | 0x00,
    dm_basic_int32 = dm_basic_int | 0x01,

    dm_basic_var_pos = dm_basic_var | 0x00,
    dm_basic_var_neg = dm_basic_var | 0x01,

    dm_type_len = 0x00,
    dm_type_val = 0x08,

    dm_type_int = dm_type_val | 0x00,
    dm_type_link = dm_type_val | 0x04,

    dm_type_string = dm_type_len | 0x00,
    dm_type_compose = dm_type_len | 0x04,
} dm_types;

// Data Tree
typedef struct dm_node_st dm_node;
typedef struct dm_tree_st dm_tree;

struct dm_node_st {
    dm_node *childs[2];                     // Position in the tree
    uint8_t  color;                         // Position in the tree

    const void *obj;                        // Object identifier
    uint64_t    pos;                        // Position in the tree

};
struct dm_tree_st {
    dm_node *root;                          // Root node of the tree
    uint64_t count;                         // Number of nodes in the tree

    dm_node *parents[RBTREE_STACK];
    uint8_t  sides  [RBTREE_STACK];
};


void dm_tree_init(dm_tree *tree) {
    tree->root = NULL;
    tree->count = 0;
}
void dm_tree_free(dm_tree *tree) {
    dm_node *next = tree->root;
    int pos = -1;

    while (1) {
        while (next) {
            tree->parents[++pos] = next;
            next = next->childs[0];
        }
        if (pos == -1) break;
        dm_node *node = tree->parents[pos--];
        next = node->childs[1];

        mem_free(node);
    }

    tree->root = NULL;
}

uint64_t dm_tree_find_pos(const dm_tree *tree, const void    *obj) {
    const dm_node *node = tree->root;

    while (node) {
        if (obj == node->obj) return node->pos;
        node = node->childs[node->obj < obj];
    }
    return 0;
}
void    *dm_tree_find_obj(const dm_tree *tree, const uint64_t pos) {
    const dm_node *node = tree->root;

    while (node) {
        if (pos == node->pos) return (void *)node->obj;
        node = node->childs[node->pos < pos];
    }
    return NULL;
}

void dm_tree_optimize(dm_tree *tree, int8_t pos) {
    while (--pos >= 0) {
        const uint8_t side = tree->sides[pos];
        dm_node *g_ = tree->parents[pos]; // Grand Parent
        dm_node *y_ = g_->childs[1 - side]; // Unlce
        dm_node *x_ = tree->parents[pos + 1]; // Parent

        if (x_->color == RBTREE_BLACK) break;
        if (y_ && y_->color == RBTREE_RED) {
            x_->color = RBTREE_BLACK;
            y_->color = RBTREE_BLACK;
            g_->color = RBTREE_RED;

            --pos;
            continue;
        }

        if (side == 1 - tree->sides[pos + 1]) {
            y_ = x_->childs[1 - side]; // y_ is child
            x_->childs[1 - side] = y_->childs[side];
            y_->childs[side] = x_;
            x_ = g_->childs[side] = y_;
        }
        g_->color = RBTREE_RED;
        x_->color = RBTREE_BLACK;
        g_->childs[side] = x_->childs[1 - side];
        x_->childs[1 - side] = g_;

        if (pos == 0) tree->root = x_;
        else tree->parents[pos - 1]->childs[tree->sides[pos - 1]] = x_;
        break;
    }

    tree->root->color = RBTREE_BLACK;
}
void dm_tree_insert_by_pos(dm_tree *tree, const void *_obj, const uint64_t _pos) {
    dm_node *node = tree->root;
    uint8_t side = 0;
    int8_t pos = -1;

    while (node && pos < RBTREE_STACK) {
        if (_pos == node->pos) return;
        tree->parents[++pos] = node;
        side = tree->sides[pos] = node->pos < _pos;
        node = node->childs[side];
    }
    if (pos == RBTREE_STACK) return;

    dm_node *new_node = mem_calloc(1, sizeof(dm_node));
    *new_node = (dm_node){{NULL, NULL}, RBTREE_RED, _obj, _pos};

    if (pos == -1) tree->root = new_node;
    else tree->parents[pos]->childs[side] = new_node;

    dm_tree_optimize(tree, pos);
}
void dm_tree_insert_by_obj(dm_tree *tree, const void *_obj, const uint64_t _pos) {
    dm_node *node = tree->root;
    uint8_t side = 0;
    int8_t pos = -1;

    while (node && pos < RBTREE_STACK) {
        if (_pos == node->pos) return;
        tree->parents[++pos] = node;
        side = tree->sides[pos] = node->obj < _obj;
        node = node->childs[side];
    }
    if (pos == RBTREE_STACK) return;

    dm_node *new_node = mem_calloc(1, sizeof(dm_node));
    *new_node = (dm_node){{NULL, NULL}, RBTREE_RED, _obj, _pos};

    if (pos == -1) tree->root = new_node;
    else tree->parents[pos]->childs[side] = new_node;

    dm_tree_optimize(tree, pos);
}



// dm_encode
typedef struct dm_oen_st dm_oen;

struct dm_oen_st {
    uint8_t type;

    uint8_t *data;
    uint64_t size;

    dm_oen *next;
};
struct dm_encode_st {
    dm_oen *first;
    dm_oen *last;

    uint64_t size;
    dm_tree *tree;                         // Pointer to the tree used during encodeing
};

void dm_encode_clear(dm_encode *ctx) {
    for (dm_oen *ptr = ctx->first, *next; ptr; ptr = next) {
        next = ptr->next;
        mem_free(ptr);
    }
    ctx->first = ctx->last = NULL;
    ctx->size = 0;
}
void dm_encode_init (dm_encode *ctx) {
    memset(ctx, 0, sizeof(dm_encode));

    dm_tree_init(ctx->tree = mem_calloc(1, sizeof(dm_tree)));
}
void dm_encode_free (dm_encode *ctx) {
    dm_encode_clear(ctx);
    dm_tree_free(ctx->tree);
}

uint64_t dm_encode_size(const dm_encode *ctx) {
    return ctx->size;
}
uint64_t dm_encode_data(const dm_encode *ctx, uint8_t *data, const uint64_t size, const uint64_t offset) {
    const dm_oen *ptr = ctx->first;
    uint64_t pos = 0;
    for (;ptr && pos + 1 + ptr->size <= size; pos += ptr->size + 1, ptr = ptr->next) {
        if (pos < offset) continue;
        data[pos] = ptr->type;
        memcpy(data + pos + 1, ptr->data, ptr->size);
    }
    return pos - offset;
}


static __inline__ uint64_t dm_encode_basic(uint64_t _val, uint8_t *_str, uint8_t *_type) {
    uint64_t _size = (71 - __builtin_clzll(_val)) / 8;
    uint64_t n_val;

    if (_size <= 4) {
        n_val = -(uint32_t) _val;

        if ((71 - __builtin_clzll( _val)) / 7 < 4) goto end_p;
        if ((71 - __builtin_clzll(n_val)) / 7 < 4) goto end_n;
        *_type = dm_basic_int32;
    } else {
        n_val = -(uint64_t) _val;

        if ((71 - __builtin_clzll( _val)) / 7 < 8) goto end_p;
        if ((71 - __builtin_clzll(n_val)) / 7 < 8) goto end_n;
        *_type = dm_basic_int64;
    }

    for (_size = 0; _val; _val >>= 8) _str[_size++] = _val & 0xFF;
    return _size;

    end_p:
        *_type = dm_basic_var_pos;
    goto val;

    end_n:
        *_type = dm_basic_var_neg;
    _val = n_val;

    val:
        for (_size = 0; _val > 0x7F; _val >>= 7) _str[_size++] = (_val & 0x7F) | 0x80;
    _str[_size++] = _val;
    return _size;
}
static __inline__ uint8_t *dm_encode_internal(dm_encode *ctx, const uint8_t type, const uint64_t val, const uint64_t size) {
    uint8_t _str[16] = {0};
    uint8_t _type;

    const uint64_t _size = dm_encode_basic(val, _str, &_type);

    dm_oen *oen = mem_calloc(1, sizeof(dm_oen) + _size + size);
    if (oen == NULL) return NULL;

    *oen = (dm_oen){_type | type, (uint8_t *)oen + sizeof(dm_oen), _size + size, NULL};

    memcpy(oen->data, _str, _size);

    if (ctx->first == NULL) ctx->first = oen;
    else ctx->last->next = oen;
    ctx->last = oen;

    ctx->size += 1 + _size + size;
    return oen->data + _size;
}

static __inline__ int32_t dm_encode_bint(dm_encode *ctx, const uint64_t val) {
    const uint8_t *oen = dm_encode_internal(ctx, dm_type_int, val, 0);
    if (oen == NULL) return CPD_FLAG_ERR_ALLOC;
    return CPD_FLAG_SUCCESS;
}
static __inline__ int32_t dm_encode_link(dm_encode *ctx, const uint64_t val) {
    const uint8_t *oen = dm_encode_internal(ctx, dm_type_link, val, 0);
    if (oen == NULL) return CPD_FLAG_ERR_ALLOC;
    return CPD_FLAG_SUCCESS;
}
static __inline__ int32_t dm_encode_ctx (dm_encode *ctx, const dm_encode *_ctx) {
    uint8_t *oen = dm_encode_internal(ctx, dm_type_string, _ctx->size, _ctx->size);
    if (oen == NULL) return CPD_FLAG_ERR_ALLOC;
    const dm_oen *ptr = _ctx->first;
    uint64_t pos = 0;
    for (;ptr; pos += ptr->size + 1, ptr = ptr->next) {
        oen[pos] = ptr->type;
        memcpy(oen + pos + 1, ptr->data, ptr->size);
    }
    return CPD_FLAG_SUCCESS;
}

int32_t dm_encode_struct( dm_encode *ctx, void *_obj, const dm_encode_func func) {
    const uint64_t src_pos = dm_tree_find_pos(ctx->tree, _obj);
    if (src_pos) return dm_encode_link(ctx, src_pos);
    dm_tree_insert_by_obj(ctx->tree, _obj, ++ctx->tree->count);

    dm_encode _ctx = {NULL, NULL, 0, ctx->tree};

    int32_t res = CPD_FLAG_SUCCESS;
    if (func) res = func(_obj, &_ctx);
    if (res == CPD_FLAG_SUCCESS) res = dm_encode_ctx(ctx, &_ctx);

    dm_encode_clear(&_ctx);
    return res;
}
int32_t dm_encode_buff(dm_encode *ctx, const uint8_t *str, const int32_t size) {
    uint8_t *oen = dm_encode_internal(ctx, dm_type_string, size, size);
    if (oen == NULL) return CPD_FLAG_ERR_ALLOC;
    memcpy(oen, str, size);
    return CPD_FLAG_SUCCESS;
}
int32_t dm_encode_str(dm_encode *ctx, const string_t *str) {
    uint8_t *oen = dm_encode_internal(ctx, dm_type_string, str->size, str->size);
    if (oen == NULL) return CPD_FLAG_ERR_ALLOC;
    memcpy(oen, str->data, str->size);
    return CPD_FLAG_SUCCESS;
}


int32_t dm_encode_double(dm_encode *ctx, const double _double) {
    uint64_t u;
    memcpy(&u, &_double, sizeof(double));
    return dm_encode_bint(ctx, u);
}
int32_t dm_encode_float (dm_encode *ctx, const float  _float) {
    uint64_t u;
    memcpy(&u, &_float, sizeof(float));
    return dm_encode_bint(ctx, u);
}


CPD_ENCODE_CONSTRUCT(uint64_t, _uint64)
CPD_ENCODE_CONSTRUCT(uint32_t, _uint32)
CPD_ENCODE_CONSTRUCT(uint16_t, _uint16)
CPD_ENCODE_CONSTRUCT(uint8_t , _uint8 )

CPD_ENCODE_CONSTRUCT(int64_t, _int64)
CPD_ENCODE_CONSTRUCT(int32_t, _int32)
CPD_ENCODE_CONSTRUCT(int16_t, _int16)
CPD_ENCODE_CONSTRUCT(int8_t , _int8 )


// dm_decode
typedef struct dm_ode_st dm_ode;

struct dm_ode_st {
    uint8_t type;

    const
    uint8_t *data;
    uint64_t size;

    dm_ode *next;
};
struct dm_decode_st {
    dm_ode *first;                       // Pointer to the first object in the encode context
    dm_ode *last;                        // Pointer to the last object in the encode context

    uint64_t count;
    dm_tree *tree;                         // Pointer to the tree used during encodeing
};

void dm_decode_clear(dm_decode *ctx) {
    for (dm_ode *ptr = ctx->first, *next; ptr; ptr = next) {
        next = ptr->next;
        mem_free(ptr);
    }
    ctx->first = ctx->last = NULL;
    ctx->count = 0;
}
void dm_decode_init (dm_decode *ctx) {
    memset(ctx, 0, sizeof(dm_decode));

    dm_tree_init(ctx->tree = mem_calloc(1, sizeof(dm_tree)));
}
void dm_decode_free (dm_decode *ctx) {
    dm_decode_clear(ctx);
    dm_tree_free(ctx->tree);
}


static __inline__ uint64_t dm_decode_basic(const uint8_t *_str, const uint64_t _type, uint64_t *val) {
    uint64_t _val = 0;
    int64_t _neg = 1;
    uint64_t _pos = 0;
    switch (_type & dm_basic_var_neg) {
        case dm_basic_int64:
            for (uint64_t i = 0; i < 4; ++i, ++_pos) _val |= ((uint64_t) _str[_pos]) << (_pos * 8);
            __attribute__((fallthrough));
        case dm_basic_int32:
            for (uint64_t i = 0; i < 4; ++i, ++_pos) _val |= ((uint64_t) _str[_pos]) << (_pos * 8);
            break;
        case dm_basic_var_neg:
            _neg = -1;
        case dm_basic_var_pos:
            for (;_str[_pos] & 0x80; ++_pos) _val |= ((uint64_t) _str[_pos] & 0x7F) << (_pos * 7);
            _val |= ((uint64_t) _str[_pos] & 0x7F) << (_pos * 7);
            ++_pos;
            break;
        default:;
    }
    *val = _val * _neg;
    return _pos;
}

uint64_t dm_decode_count(const dm_decode *ctx) {
    return ctx->count;
}
int32_t dm_decode_data  (      dm_decode *ctx, const uint8_t *data, const uint64_t size) {
    for (uint64_t pos = 0; pos < size;) {
        dm_ode *ode = mem_calloc(1, sizeof(dm_ode));
        if (ode == NULL) return CPD_FLAG_ERR_ALLOC;

        if (ctx->count++ == 0) ctx->first = ode;
        else ctx->last->next = ode;
        ctx->last = ode;

        ode->type = data[pos++];
        if (pos == size) return CPD_FLAG_ERR_EOF;

        pos += dm_decode_basic(data + pos, ode->type, &ode->size);
        if ((ode->type & dm_type_val) == dm_type_len) {
            ode->data = data + pos;
            pos += ode->size;
        }
        if (pos > size) return CPD_FLAG_ERR_EOF;
    }
    return CPD_FLAG_SUCCESS;
}


static __inline__ int32_t dm_decode_bint(dm_decode *ctx, uint64_t *val) {
    dm_ode *ode = ctx->first;
    if (ode == NULL) return CPD_FLAG_ERR_EOF;
    if ((ode->type & 0x0c) != dm_type_int) return CPD_FLAG_ERR_TYPE;

    *val = ode->size;

    ctx->first = ode->next;
    mem_free(ode);
    return CPD_FLAG_SUCCESS;
}
static __inline__ int32_t dm_decode_link(dm_decode *ctx, void **_obj) {
    dm_ode *ode = ctx->first;
    if (ode == NULL) return CPD_FLAG_ERR_EOF;
    if ((ode->type & 0x0c) != dm_type_link) return CPD_FLAG_ERR_TYPE;

    *_obj = dm_tree_find_obj(ctx->tree, ode->size);

    ctx->first = ode->next;
    mem_free(ode);
    return CPD_FLAG_SUCCESS;
}

int32_t dm_decode_struct(dm_decode *ctx, void **_obj, const dm_decode_func func, dm_obj_new func_new) {
    if (dm_decode_link(ctx, _obj) == CPD_FLAG_SUCCESS) return CPD_FLAG_SUCCESS;

    dm_ode *ode = ctx->first;
    if (ode == NULL) return CPD_FLAG_ERR_EOF;
    if ((ode->type & 0x0c) != dm_type_compose) return CPD_FLAG_ERR_TYPE;

    if (ode->data == NULL) return CPD_FLAG_ERR_NULLPTR;

    *_obj = func_new? func_new(): NULL;
    dm_tree_insert_by_pos(ctx->tree, *_obj, ++ctx->tree->count);


    dm_decode _ctx = {NULL, NULL, 0, ctx->tree};

    int32_t res = dm_decode_data(&_ctx, ode->data, ode->size);
    if (res) goto end;
    if (func) res = func(*_obj, &_ctx);
    if (res) goto end;

    ctx->first = ode->next;
    mem_free(ode);
end:
    dm_decode_clear(&_ctx);
    return res;
}
int32_t dm_decode_buff(dm_decode *ctx, uint8_t *str, const uint64_t size, uint64_t *res_size) {
    dm_ode *ode = ctx->first;
    if (ode == NULL) return CPD_FLAG_ERR_EOF;
    if ((ode->type & 0x0c) != dm_type_string) return CPD_FLAG_ERR_TYPE;

    if (ode->data == NULL) return CPD_FLAG_ERR_NULLPTR;

    *res_size = ode->size > size ? size : ode->size;
    memcpy(str, ode->data, *res_size);

    ctx->first = ode->next;
    mem_free(ode);
    return CPD_FLAG_SUCCESS;
}
int32_t dm_decode_str(dm_decode *ctx, string_t *str) {
    dm_ode *ode = ctx->first;
    if (ode == NULL) return CPD_FLAG_ERR_EOF;
    if ((ode->type & 0x0c) != dm_type_string) return CPD_FLAG_ERR_TYPE;

    if (ode->data == NULL) return CPD_FLAG_ERR_NULLPTR;

    string_set_str(str, ode->data, ode->size);

    ctx->first = ode->next;
    mem_free(ode);
    return CPD_FLAG_SUCCESS;
}


CPD_DECODE_CONSTRUCT(double, _double)
CPD_DECODE_CONSTRUCT(float , _float )

CPD_DECODE_CONSTRUCT(uint64_t, _uint64)
CPD_DECODE_CONSTRUCT(uint32_t, _uint32)
CPD_DECODE_CONSTRUCT(uint16_t, _uint16)
CPD_DECODE_CONSTRUCT(uint8_t , _uint8 )

CPD_DECODE_CONSTRUCT(int64_t, _int64)
CPD_DECODE_CONSTRUCT(int32_t, _int32)
CPD_DECODE_CONSTRUCT(int16_t, _int16)
CPD_DECODE_CONSTRUCT(int8_t , _int8 )


