/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/* avl trees for general data */

#ifndef GAVL_H
#define GAVL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct gavl_node_type {
  int h, count;
  void *key;
  struct gavl_node_type *p, *l, *r;
} gavl_node_t;

//static int (*cmp)(void *, void *);
//static int default_cmp(void *a, void *b);

gavl_node_t *gavl_node_alloc(gavl_node_t *);
void gavl_free(gavl_node_t *p);
void gavl_deep_free(gavl_node_t *, void (*freefunc)(void *));
gavl_node_t *get_root(gavl_node_t *);
void gavl_print(gavl_node_t *, int);

void gavl_rotl(gavl_node_t *);
void gavl_rotr(gavl_node_t *);

int gavl_height(gavl_node_t *, int);
int gavl_keycheck_gt(gavl_node_t *, void *);
int gavl_keycheck_lt(gavl_node_t *, void *);
int gavl_consist(gavl_node_t *);

int gavl_insert(gavl_node_t *, void *);
gavl_node_t *gavl_succ(gavl_node_t *);
gavl_node_t *gavl_delete(gavl_node_t *, void *);




/*****************
   tree functions
 *****************/

typedef struct avl_type {
  int n;
  gavl_node_t *root;
  int (*cmp)(void *, void *);
  void (*free)(void *), (*print)(void *);
} avl_t;

avl_t *avl_alloc(void *);
avl_t *avl_alloc2(void *, int (*cmpfunc)(void *, void *), void (*freefunc)(void *));

void avl_free(avl_t *);
void avl_free2(avl_t *, void (*freefunc)(void *));

int   avl_ins(avl_t *, void *);
void *avl_del(avl_t *, void *);
void *avl_get(avl_t *, void *);

void *avl_clt(avl_t *, void *);
void *avl_clte(avl_t *, void *);

void *avl_cgt(avl_t *, void *);
void *avl_cgte(avl_t *, void *);

void avl_print(avl_t *a); 
void avl_print2(avl_t *a, void (*printfunc)(void *));

#endif
