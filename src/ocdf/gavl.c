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

/* avl trees.
 * kind of tested....still a little iffy
 *
 * might still have a memory leak
 *
 * */
#include "avl.h"

static int (*cmp)(void *, void *);

static int default_cmp(void *a, void *b) {
  int x, y;
  x=(long int)a; y=(long int)b;
  return(a-b);
}

void gavl_set_cmp( int (*c)(void *, void *)) { cmp=c; }

gavl_node_t *gavl_node_alloc(gavl_node_t *par) {
  gavl_node_t *nn;
  nn=malloc(sizeof(gavl_node_t));
  nn->h=nn->count=0; nn->key=NULL; nn->p=par; nn->l=nn->r=NULL;
  return(nn);
}


void gavl_free(gavl_node_t *p) {
  if (!p) return;
  if (p->l) gavl_free(p->l);
  if (p->r) gavl_free(p->r);
  free(p);
}

static void (*loc_printfunc)(void *);

void gavl_deep_free(gavl_node_t *p, void (*freefunc)(void *)) {
  void *k;
  if (!p) return; k=p->key; 
  if (p->l) gavl_deep_free(p->l, freefunc); 
  if (p->r) gavl_deep_free(p->r, freefunc);

  if (k) (*freefunc)(k);
  free(p);
}

gavl_node_t *get_root(gavl_node_t *p) {
  if (p->p==NULL) return(p);
  return(get_root(p->p));
}


void gavl_print(gavl_node_t *p, int level) {
  int i, j, k;
  if (p==NULL) return;
  gavl_print(p->r, level+1);
  for (i=0; i<level; i++) printf("   ");
  printf("[%i]%s: %p %c%s (par %p) [%p, %p]\n", level, 
     (p->p==NULL) ? "*" : "", p->key,
     (p->h<0) ? '-' : ( (p->h==0) ? '0' : '+' ),
     (p->h==2) ? "+" : ( (p->h==-2) ? "-" : ""),
     (p->p) ? p->p->key : NULL,
     p->l ? p->l->key : 0, p->r ? p->r->key : 0);
  gavl_print(p->l, level+1);
}

static void default_print(void *p) { printf("(%p)\n", p); }


void gavl_uprint(gavl_node_t *p, void (*user_print)(void *), int level) {
  int i, j, k;
  if (!user_print) user_print=default_print;
  if (p==NULL) return;
  gavl_uprint(p->r, user_print, level+1);
  for (i=0; i<level; i++) printf(" "); (*user_print)(p->key); printf("\n");
  gavl_uprint(p->l, user_print, level+1);
}

void gavl_rotl(gavl_node_t *p) {
  gavl_node_t *t, *pp, *q; 
  if (p->r==NULL) { printf("gavl_rotl: sanity, p->r NULL (k %p, p %p)\n",p->key,p->p?p->p->key:NULL);exit(0); }
  pp=p->p; q=p->r;

  //printf("rotl: par %i, p %i, q %i\n", pp ? pp->key : -1, p->key, q->key);

  t=q->l; q->l=p; p->r=t; p->p=q; q->p=pp;
  if (t) t->p=p;

  if (!pp) return;

  if (pp->r==p) pp->r=q;
  else pp->l=q;
}

void gavl_rotr(gavl_node_t *p) {
  gavl_node_t *t, *pp, *q;
  if (p->l==NULL) { printf("gavl_rotr: sanity p->l NULL (k %p, p %p)\n",p->key,p->p?p->p->key:NULL);exit(0); }
  pp=p->p; q=p->l;

  //printf("rotr: par %i, p %i, q %i\n", pp ? pp->key : -1, p->key, q->key);

  t=q->r; q->r=p; p->l=t; p->p=q; q->p=pp;
  if (t) t->p=p;

  if (!pp) return;

  if (pp->r==p) pp->r=q;
  else pp->l=q;
}

int gavl_height(gavl_node_t *p, int h) {
  int a=0, b=0; gavl_node_t *t;
  if (!p) { return(h-1); }
  a=gavl_height(p->l, h+1);
  b=gavl_height(p->r, h+1);
  return( (a>b) ? a : b );
}

int gavl_keycheck_gt(gavl_node_t *p, void *key) {
  int a, b;
  if (!p) return(1);
  //if (p->key>key) { 
  if ( (*cmp)(p->key, key) > 0 ) {
    printf("gt key failed (%li)\n", (long int)p->key); 
    printf("tree state\n");
    gavl_print(get_root(p), 0);
    return(0); 
  }
  a=gavl_keycheck_gt(p->l, key);
  b=gavl_keycheck_gt(p->r, key);
  return( (a==1) && (b==1) );
}

int gavl_keycheck_lt(gavl_node_t *p, void *key) {
  int a, b;
  if (!p) return(1);
  //if (p->key<=key) {
  if ( (*cmp)(p->key, key) < 0 ) {
    printf("lt key failed (%li)\n", (long int)p->key); 
    printf("tree state\n");
    gavl_print(get_root(p), 0);
    return(0); 
  }
  a=gavl_keycheck_lt(p->l, key);
  b=gavl_keycheck_lt(p->r, key);
  return( (a==1) && (b==1) );
}

int lchild(gavl_node_t *p) { if (p->p) return(p->p->l==p); return(-1); }
int rchild(gavl_node_t *p) { if (p->p) return(p->p->r==p); return(-1); }

int gavl_consist(gavl_node_t *p) {
  int a, b, c; gavl_node_t *t; FILE *fp=stdout;
  if (!p) return(1);
  a=gavl_height(p->l, 1);
  b=gavl_height(p->r, 1);
  //printf("   node %i (%i) got height %i %i\n", p->key, p->h, a, b);
  if ((b-a)!=p->h) { 
    fprintf(fp, "height sanity\n"); 
    printf("tree state\n");
    gavl_print(get_root(p), 0);
    return(0);
  }

  if (p->p) {
    if (lchild(p) && p->p->l!=p) { fprintf(fp, "par consist fail 1\n"); return(0); }
    if (rchild(p) && p->p->r!=p) { fprintf(fp, "par consist fail 2\n"); return(0); }
  }
  if ((p->l) && (p->l->p!=p)) { fprintf(fp, "lchild consist fail\n"); return(0); }
  if ((p->r) && (p->r->p!=p)) { fprintf(fp, "rchild consist fail\n"); return(0); }

  if (!gavl_keycheck_gt(p->l, p->key)) { fprintf(fp, "key fail 1\n"); return(0); }
  if (!gavl_keycheck_lt(p->r, p->key)) { fprintf(fp, "key fail 2\n"); return(0); }
  a=gavl_consist(p->l);
  b=gavl_consist(p->r);
  return( (a==1) && (b==1) );
}

/* returns 1 if height changed 0 otherwise
 * -1 on duplicate key
   ain't pretty but it works
   */
int gavl_insert(gavl_node_t *p, void *key) {
  int a, k; gavl_node_t *q, *t;

  if (!(q=t=p)) return(0); 

  while (t) { 
    q=t;
    if ( (*cmp)(q->key, key) == 0) { t->count++; return(-1); } 
    t=( ((*cmp)(key,t->key) < 0) ? t->l : t->r ); 
  }

  if ( (*cmp)(key,q->key) < 0 ) { t=q->l=gavl_node_alloc(q); }
  else { t=q->r=gavl_node_alloc(q); }
  t->key=key;


  while (1) {
    if (t==p) break; if (lchild(t)) q->h--; else if (rchild(t)) q->h++;
    if (q->h==0) break; if ((q->h==1) || (q->h==-1)) { t=q; q=q->p; continue; }
    if (q->h==2) {
      if (t->h==1) { gavl_rotl(q); t->h=q->h=0; }  //++ +
      else { //++ -
        gavl_rotr(t); gavl_rotl(q);
        if (t->p->h==1) { t->h=0; q->h=-1; }
        else if (t->p->h==-1) { t->h=1; q->h=0; }
        else {  t->h=q->h=0; }
        q->p->h=0;
      }
    } else {
      if (t->h==1) { //-- +
        gavl_rotl(t); gavl_rotr(q);
        if (t->p->h==1) { t->h=-1; q->h=0; }
        else if (t->p->h==-1) { t->h=0; q->h=1; }
        else { t->h=q->h=0; }
        q->p->h=0;
      } else { gavl_rotr(q); t->h=q->h=0; } //-- -
    }
    break;
  }
  return(0);
}

gavl_node_t *gavl_succ(gavl_node_t *p) {
  gavl_node_t *t=NULL;
  if (p->r) { t=p->r; while (t->l) t=t->l; }
  else if (p->l) { t=p->l; while (t->r) t=t->r; }
  return(t);
}

/* return deleted node, null on not found
 * 
 * gotta be carefull if its the root thats deleted...
 * root should have pointers into valid tree still...
 *
 */
gavl_node_t *gavl_delete(gavl_node_t *root, void *key) {
  int a, k, upsub; gavl_node_t *p, *q, *t, *ret, *cur_root;
  int debug=0; void *voidoid;

  if (!(q=t=root)) return(NULL);
  if ((root->l==NULL) && (root->r==NULL)) {
    if ((*cmp)(root->key, key)==0) { return(root); } 
    else return(NULL);
  }

  //while (t && (t->key!=key)) t=( (key<t->key) ? t->l : t->r ); 
  while (t && ((*cmp)(t->key,key)!=0) ) t=( ((*cmp)(key,t->key)<0) ? t->l : t->r ); 
  if (!t) return(NULL);
  ret=t; //t=gavl_succ(t); 

  upsub=1;
  if ((t->r==NULL) && (t->l==NULL)) {               // leaf

    if (debug) printf("leaf removal\n");

    if (lchild(t)) { t->p->l=NULL; t->p->h++; }
    else { t->p->r=NULL; t->p->h--; }
    p=t->p; q=NULL;
  } else if (t->r==NULL) {                          // channel left

    if (debug) printf("channel left removal\n");
    
    if (t->p && lchild(t)) { t->p->l=t->l; t->p->h++; }
    else if (t->p) { t->p->r=t->l; t->p->h--; }
    t->l->p=t->p;
    p=t->p; q=t->l;
  } else if (t->l==NULL) {                          // channel right
    
    if (debug) printf("channel right removal\n");

    if (t->p && lchild(t)) { t->p->l=t->r; t->p->h++; }
    else if (t->p) { t->p->r=t->r; t->p->h--; }
    t->r->p=t->p;
    p=t->p; q=t->r;
  } else if (t->r->l==NULL) {                       // Y

    if (debug) printf("Y removal\n");

    if (lchild(t)) { 

      if (debug) printf("  left child\n");

      if (t->p) t->p->l=t->r; 
      t->r->p=t->p; t->l->p=t->r; t->r->l=t->l;
      ret=t;
      if (t->h==1) t->r->h=0;
      else if (t->h==0) t->r->h=-1;
      else if (t->h==-1) t->r->h=-2;
      else { //sanity
        printf("sanity 1\n");
        gavl_print(root, 0);
      }
      p=t->r; q=t->r->l;
    } else {

      if (debug) printf("  right child\n");

      if (t->p) t->p->r=t->r; 
      t->r->p=t->p; t->l->p=t->r; t->r->l=t->l;
      if (t->h==1) t->r->h=0;
      else if (t->h==0) t->r->h=-1;
      else if (t->h==-1) t->r->h=-2;
      else { //sanity
        printf("sanity 2\n");
        gavl_print(root, 0);
      }
      p=t->r; q=t->r->l;
    }
  } else {                                          // patch up successor

    if (debug) printf("   normal succ match\n");

    t=gavl_succ(t);
    t->p->l=t->r;
    if (t->r) t->r->p=t->p;
    voidoid=t->key; t->key=ret->key; ret->key=voidoid;
    t->p->h++;
    p=t->p; q=t->r;
  }
  ret=t;

  cur_root= ( p ? get_root(p) : root );

  if (debug) {
    printf("ret %p, t %p\n", ret->key, t ? t->key : NULL);
    printf("state after initial patchup:\n");
  }


  //
  //p holds altered node
  while (p && upsub) {

    if (debug) {
      printf("loop key %p [h %i], upsub %i\n", p->key, p->h, upsub); fflush(stdout);
      printf("\n\n");
      gavl_print(cur_root, 0);
      printf("\n\n");
    }

    switch (p->h) {
      case 1: if (debug) printf("case 1\n"); 
      case -1: if (debug) printf("case -1\n"); 
              upsub=0; break;
      case 0: if (debug) { printf("case 0 p %p, key %p\n", p, p ? p->key : 0); fflush(stdout); }
              if (p->p) { if (lchild(p)) p->p->h++; else p->p->h--; } p=p->p; 
              break;
      case 2: if (debug) printf("case 2\n");
              //if ((p->r->l) && (p->r->h==-1)) { //++ -
              if (p->r->h==-1) {

                if (debug) printf(" ++ -\n");

                gavl_rotr(p->r); gavl_rotl(p); p=p->p; if (!p) break;
                if      (p->h ==  1) { p->l->h = -1; p->r->h = 0; p->h=0; }
                else if (p->h == -1) { p->l->h =  0; p->r->h = 1; p->h=0; }
                else if (p->h ==  0) { p->l->h =  0; p->r->h = 0; p->h=0; }
                else { printf("delete sanity case 2\n"); return(0); }
              } else { //++ +
                
                if (debug) printf(" ++ +\n");

                gavl_rotl(p); p=p->p; if (!p) break;
                if      (p->h==1) { p->l->h=0; p->h= 0; }
                else if (p->h==0) { p->l->h=1; p->h=-1; upsub=0; }
                else { printf("delete sanity case 2b\n"); return(0); }
              }
              if (p->p && upsub) { if (lchild(p)) p->p->h++; else p->p->h--; }
              p=p->p;
              break;
      case -2: if (debug) printf("case -2\n");

              //if ((p->l->r) && (p->l->h==1)) {  //-- +
              if (p->l->h==1) {

                if (debug) printf(" -- +\n");

                gavl_rotl(p->l); gavl_rotr(p); 

                /*
                printf(" tree after rotates, but before height changes\n");
                printf(" p %i\n", p->key);
                gavl_print(get_root(p), 0);
                printf("\n\n");
                */
                
                p=p->p; if (!p) break;
                if      (p->h ==  1) { p->l->h = -1; p->r->h = 0; p->h = 0; }
                else if (p->h == -1) { p->l->h =  0; p->r->h = 1; p->h = 0; }
                else if (p->h ==  0) { p->l->h =  0; p->r->h = 0; p->h = 0; } 
                else { printf("delete sanity case -2\n"); return(0); }
              } else { //-- -

                if (debug) printf(" -- -\n");

                gavl_rotr(p); p=p->p; if (!p) break;
                if      (p->h == -1) { p->r->h =  0; p->h = 0; }
                else if (p->h ==  0) { p->r->h = -1; p->h = 1; upsub=0; }
                else { printf("delete sanity case -2b\n"); return(0); }
              }
              if (p->p && upsub) { if (lchild(p)) p->p->h++; else p->p->h--; }
              p=p->p;
              break;
      default: printf("delete case sanity\n"); return(0); break;
    }
  }
  return(ret);
}


/*************************************************************************
 * avl tree functions 
 *************************************************************************/

avl_t *avl_alloc(void *p) {
  avl_t *a;
  a = malloc(sizeof(avl_t));
  a->cmp=NULL; a->free=free; a->print=NULL;
  a->root=p; a->n=(p?1:0);
  return(a);
}

avl_t *avl_alloc2(void *p, int (*cmpfunc)(void *, void *), void (*freefunc)(void *)) {
  avl_t *a;
  a=avl_alloc(p);
  a->cmp=cmpfunc; a->free=freefunc;
}

void avl_free(avl_t *a) {
  if (!a->free) { printf("SANITY IN FREE, no free func...\n"); exit(0); }
  gavl_deep_free(a->root, a->free);
  free(a);
}

void avl_free2(avl_t *a, void (*freefunc)(void *)) { gavl_deep_free(a->root, freefunc); }



/* returns 1 on height change, 0 no height change
 * -1 on duplicate key
 */
int avl_ins(avl_t *a, void *p) {
  int ret; 

  if (!a->cmp) { printf("sanity!  must provide cmp fun\n"); exit(0); }

  cmp = a->cmp;

  if (!a->root) { 
    a->root=gavl_node_alloc(a->root); 
    a->root->key=p;
    return(1); 
  }
  ret=gavl_insert(a->root, p);

  a->root = get_root(a->root);
  return(ret);
}




/* NULL on error, key data otherwise */
void *avl_del(avl_t *a, void *p) {
  gavl_node_t *g; void *v=NULL;

  if (!a->cmp) { printf("sanity!  must provide cmp fun\n"); exit(0); }

  if (!a) return(NULL);
  if (!a->root) return(NULL);

  cmp       = a->cmp;
  g         = gavl_delete(a->root, p);
  a->root   = get_root(a->root);

  if (g) { 
    if (g==a->root)  {
      a->root = (a->root->r ? a->root->r : a->root->l);
      if (a->root) a->root=get_root(a->root);
    }
    v=g->key;  free(g); 
  }
  return(v);
}

void *avl_get_r(avl_t *a, gavl_node_t *g, void *p) {
  int k;
  if (!g) return(NULL);
  if ( (k=((*(a->cmp))(p,g->key))) == 0 ) return(g->key);
  else if (k<0) return(avl_get_r(a, g->l, p));
  else return(avl_get_r(a, g->r, p));
}


void *avl_get(avl_t *a, void *p) {
  gavl_node_t *g; void *v=NULL;
  if (!a) return(NULL);
  if (!a->cmp) { printf("sanity! must provide cmp func\n"); exit(0); }
  if (!a->root) return(NULL);
  if (!p) return(NULL);
  cmp       = a->cmp;
  return(avl_get_r(a, a->root, p));
}


/* closest greater than or equal */
void *avl_cgte(avl_t *a, void *p) {
  int i, j, k, val=1;
  gavl_node_t *g, *cur, *prev; void *v=NULL;

  if ((!a) || (!a->cmp) || (!a->root) || (!p)) return(NULL);

  cmp = a->cmp; prev = cur = a->root; val = (*cmp)(p, cur->key);
  while (cur && (val!=0)) {
    prev = cur;
    cur = ( (val<0) ? cur->l : cur->r );
    if (cur) val=(*cmp)(p, cur->key);
  }

  if (cur) return(cur->key);
  if (!prev->p) return(prev->key);

  if (val<0) return(prev->key);

  cur=prev; 
  while (cur && (rchild(cur))) {
    prev=cur; cur=cur->p;
    if (!cur->p) return(NULL);
  }

  return(cur->p->key);
}

/* closest less than or equal */
void *avl_clte(avl_t *a, void *p) {
  int i, j, k, val=1;
  gavl_node_t *g, *cur, *prev; void *v=NULL;

  if ((!a) || (!a->cmp) || (!a->root) || (!p)) return(NULL);

  cmp = a->cmp; prev = cur = a->root; val = (*cmp)(p, cur->key);
  while (cur && (val!=0)) {
    prev = cur;
    cur = ( (val<0) ? cur->l : cur->r );
    if (cur) val=(*cmp)(p, cur->key);
  }

  if (cur) return(cur->key);
  if (!prev->p) return(prev->key);

  if (val>0) return(prev->key);

  cur=prev; 
  while (cur && (lchild(cur))) {
    prev=cur; cur=cur->p;
    if (!cur->p) return(NULL);
  }

  return(cur->p->key);
}


/* closest greater than */

void avl_tree_set_cmp(avl_t *a, int (*cmpfunc)(void *, void *)) { a->cmp=cmpfunc; }
void avl_tree_set_free(avl_t *a, void (*freefunc)(void *)) { a->free=freefunc; }
void avl_tree_set_print(avl_t *a, void (*printfunc)(void *)) { 

  loc_printfunc=printfunc;

  a->print=printfunc; 
}

void avl_print_r(avl_t *a, gavl_node_t *p, int level) {
  int i, j, k;

  if (p==NULL) return;
  avl_print_r(a, p->r, level+1);

  for (i=0; i<level; i++) printf("  ");
  if (a->print) { printf("%p ", p); (*(a->print))(p->key); }
  else printf("%p", p->key);
  printf("\n");

  avl_print_r(a, p->l, level+1);
}


void avl_print(avl_t *a) { avl_print_r(a, a->root, 0); }


/*************************************************************************
 * avl tree functions 
 *************************************************************************/


