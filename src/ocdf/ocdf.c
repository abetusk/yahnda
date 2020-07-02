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

/* cumulative distribution function
 * i.e. running total of input function
 * also bins
 *
 * takes in floating point rep, converts to rational
 * outputs fixed floating point?
 */
#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <avl.h>

#include <getopt.h>

#define buflen 4096

int renormalize_flag, data_technique, df_flag;
FILE *ifp, *ofp;

typedef struct xypair_type {
  mpq_t x, y;
} pnt_t;

int curprec;
mpf_t flo;

typedef struct ctx_bin_type {
  mpq_t curx, curmin, curmax, dx_inv;
  mpq_t fac, cursum, n;
  mpf_t tf, tf2; pnt_t tp;
  mpq_t tq, tq2, tot;
} ctx_bin_t;

void ctx_bin_init(ctx_bin_t *m) {
  mpq_init(m->curx); mpq_init(m->curmin); mpq_init(m->curmax);
  mpq_init(m->fac);  mpq_init(m->cursum); mpq_init(m->n);
  mpq_init(m->tq);   mpq_init(m->tq2);    mpq_init(m->tot);
  mpq_init(m->tp.x); mpq_init(m->tp.y);
  mpf_init(m->tf);   mpf_init(m->tf2);
  mpq_init(m->dx_inv);
}

void ctx_bin_free(ctx_bin_t *m) {
  mpq_clear(m->curx); mpq_clear(m->curmin); mpq_clear(m->curmax);
  mpq_clear(m->fac);  mpq_clear(m->cursum); mpq_clear(m->n);
  mpq_clear(m->tq);   mpq_clear(m->tq2);    mpq_clear(m->tot);
  mpq_clear(m->tp.x); mpq_clear(m->tp.y);
  mpf_clear(m->tf);   mpf_clear(m->tf2);
  mpq_clear(m->dx_inv);
  free(m);
}

void calc_tot_r(gavl_node_t *g, mpq_t tot) {
  if (!g) return;
  mpq_add(tot, tot, ((pnt_t *)(g->key))->y);
  calc_tot_r(g->l, tot);
  calc_tot_r(g->r, tot);
}

int cmp(void *a, void *b) { return(mpq_cmp( ((pnt_t *)a)->x, ((pnt_t *)b)->x )); }
void freefunc(void *a) { if (a) { mpq_clear(((pnt_t *)a)->x); mpq_clear(((pnt_t *)a)->y); free(a); } }
void printfunc(void *a) {
  int i, j, k; pnt_t *w;
  w=(pnt_t *)a;
  if (mpz_cmp_ui(mpq_denref(w->x), 1)==0) { mpq_out_str(ofp, 10, w->x); } 
  else { mpf_set_q(flo, w->x); mpf_out_str(ofp, 10, mpf_get_prec(flo), flo); }
  fprintf(ofp, " ");
  if (mpz_cmp_ui(mpq_denref(w->y), 1)==0) { mpq_out_str(ofp, 10, w->y); } 
  else { mpf_set_q(flo, w->y); mpf_out_str(ofp, 10, mpf_get_prec(flo), flo); }
}

void cdf_calc_r(avl_t *cdf, gavl_node_t *n, mpq_t sum, int level) {
  int i, j, k; pnt_t *kd;

  if (!n) return;

  cdf_calc_r(cdf, n->l, sum, level+1);

  kd=malloc(sizeof(pnt_t));
  mpq_init(kd->x); mpq_init(kd->y);
  mpq_set(kd->x, ((pnt_t *)n->key)->x);
  mpq_add(kd->y, ((pnt_t *)n->key)->y, sum);
  mpq_set(sum, kd->y);

  k=avl_ins(cdf, kd);
  if (k<0) freefunc(kd);

  cdf_calc_r(cdf, n->r, sum, level+1);
}

void cdf_calc(avl_t *cdf, avl_t *f) { 
  mpq_t sum;
  mpq_init(sum); /* default init to 0/1 */
  cdf_calc_r(cdf, f->root, sum, 0);
  mpq_clear(sum);
}

void print_cdf_r(avl_t *avl, gavl_node_t *n, mpq_t sum, mpq_t renorm, int level) {
  static pnt_t *tpnt=NULL; 
  
  if (!tpnt) { tpnt=malloc(sizeof(pnt_t)); mpq_init(tpnt->x); mpq_init(tpnt->y); }

  if (!n) return;
  print_cdf_r(avl, n->l, sum, renorm, level+1);
  mpq_add(sum, sum, ((pnt_t *)n->key)->y);

  mpq_set(tpnt->x, ((pnt_t *)n->key)->x); 
  mpq_set(tpnt->y, sum);
  if (renormalize_flag && (mpq_cmp_ui(renorm, 0, 1)>0) )
    mpq_div(tpnt->y, tpnt->y, renorm);

  printfunc(tpnt); fprintf(ofp, "\n");

  print_cdf_r(avl, n->r, sum, renorm, level+1);
}

void print_cdf(avl_t *avl) {
  mpq_t sum, renorm; 
  
  mpq_init(sum);
  mpq_init(renorm);

  if (renormalize_flag) {
    mpq_set_ui(renorm, 0, 1);
    calc_tot_r(avl->root, renorm);
  }

  print_cdf_r(avl, avl->root, sum, renorm, 0);

  mpq_clear(sum);
  mpq_clear(renorm);
}

/*************************
 *     x                  x
 *     |                  |
 *     ____________________
 *     | bin 0  |  bin 1  |
 *     |________|_________| 
 *
 * x   0                  1
 *
 *************************/
void print_bin_r(gavl_node_t *n, ctx_bin_t *m) {
  if (!n) return;

  print_bin_r(n->l, m);

  while (mpq_cmp(m->curx, ((pnt_t *)n->key)->x)<0) {
    mpq_set(m->tp.x, m->curx);
    mpq_set(m->tp.y, m->cursum);
    if (renormalize_flag && (mpq_cmp_ui(m->tot, 0, 1)>0)) {
      mpq_div(m->tp.y, m->tp.y, m->tot);
      if (df_flag) 
        mpq_mul(m->tp.y, m->tp.y, m->dx_inv);
    }
    printfunc(&(m->tp)); fprintf(ofp, "\n");
    mpq_set_ui(m->cursum, 0, 1);
    mpq_add(m->curx, m->curx, m->fac);

  }
  mpq_add(m->cursum, m->cursum, ((pnt_t *)n->key)->y);

  print_bin_r(n->r, m);

}


void print_bin(avl_t *avl, int b) {
  gavl_node_t *tn; ctx_bin_t *m;

  if (!avl->root) return;

  m = malloc(sizeof(ctx_bin_t));
  ctx_bin_init(m);

  if (renormalize_flag) {
    mpq_set_ui(m->tot, 0, 1);
    calc_tot_r(avl->root, m->tot);
  }

 
  tn=avl->root; while (tn->l) tn=tn->l;
  mpq_set(m->curmin, ((pnt_t *)tn->key)->x);
  mpq_set(m->curx, m->curmin);

  tn=avl->root; while(tn->r) tn=tn->r;
  mpq_set(m->curmax, ((pnt_t *)tn->key)->x);

  mpq_set_ui(m->tq, 1, b);
  mpq_sub(m->fac, m->curmax, m->curmin);
  mpq_mul(m->fac, m->fac, m->tq);
  mpq_add(m->curx, m->curx, m->fac);

  mpq_set_ui(m->dx_inv, b, 1);
  //mpq_div(m->dx_inv, m->dx_inv, m->fac);
  
  print_bin_r(avl->root, m);

  mpq_set(m->tp.x, m->curmax);
  mpq_set(m->tp.y, m->cursum);
  if (renormalize_flag && (mpq_cmp_ui(m->tot, 0, 1)>0))  {
    mpq_div(m->tp.y, m->tp.y, m->tot);
    if (df_flag) 
      mpq_mul(m->tp.y, m->tp.y, m->dx_inv);
  }
  printfunc(&(m->tp));
  fprintf(ofp, "\n");

  ctx_bin_free(m);

}

void printfunc2(ctx_bin_t *m) {
  /* tf = x
   * tf2 = y
   */

  if (data_technique==0) {

    /* x = sqrt( a^{j-1} * a^{j} ) */
    mpq_div(m->tq2, m->curx, m->fac);
    mpq_mul(m->tq2, m->tq2, m->curx);
    mpf_set_q(m->tf, m->tq2);
    mpf_sqrt(m->tf, m->tf);

  } else if (data_technique==1) {

    /* x = sqrt( curmin * curmax ) */
    mpq_mul(m->tq2, m->curmin, m->curmax);
    mpf_set_q(m->tf, m->tq2);
    mpf_sqrt(m->tf, m->tf);

  } else if (data_technique==2) {

    /* x = right side of (exponential) window */
    mpq_div(m->tq2, m->curx, m->fac);
    mpf_set_q(m->tf, m->tq2);

  }


  if (data_technique==0) {

    /* tq = (a^{j+1} - a^j + 1) */
    mpq_div(m->tq2, m->curx, m->fac);
    mpq_sub(m->tq, m->curx, m->tq2);
    mpq_set_ui(m->tq2, 1, 1);
    mpq_add(m->tq, m->tq, m->tq2);

  } else if (data_technique==1) {

    /* tq = curmax - curmin + 1 */
    mpq_sub(m->tq, m->curmax, m->curmin);
    mpq_set_ui(m->tq2, 1, 1);
    mpq_add(m->tq, m->tq, m->tq2);

  } else if (data_technique==2) {

    /* no division factor */
    mpq_set_ui(m->tq, 1, 1);

  }

  /* y = tot / n * (curmax - curmin + 1) */

  /*
  printf("\n\n");
  printf("curmax ");  mpq_out_str(stdout, 10, m->curmax);  printf("\n");
  printf("curmin ");  mpq_out_str(stdout, 10, m->curmin);  printf("\n");
  printf("tot    ");  mpq_out_str(stdout, 10, m->tot);     printf("\n");
  printf("tq     ");  mpq_out_str(stdout, 10, m->tq);      printf("\n");
  printf("cursum ");  mpq_out_str(stdout, 10, m->cursum);  printf("\n");
  */

  if (renormalize_flag) 
    mpq_mul(m->tq, m->tq, m->tot);
  mpq_div(m->tq, m->cursum, m->tq);
  mpf_set_q(m->tf2, m->tq);

  mpf_out_str(ofp, 10, mpf_get_prec(m->tf), m->tf);
  fprintf(ofp, " ");
  mpf_out_str(ofp, 10, 0, m->tf2);
  fprintf(ofp, "\n");

}


void print_mult_bin_r(gavl_node_t *n, ctx_bin_t *m) {
  if (!n) return;

  print_mult_bin_r(n->l, m);
  while (mpq_cmp( ((pnt_t *)n->key)->x, m->curx) >= 0) {

    //if (mpq_cmp( ((pnt_t *)n->key)->x, m->curx) <= 0)
    if (mpq_cmp_ui(m->cursum, 0, 1) > 0) 
      printfunc2(m);
    mpq_mul(m->curx, m->curx, m->fac);

    mpq_set_ui(m->cursum, 0, 1);
    mpq_set(m->curmin, ((pnt_t *)n->key)->x);
    mpq_set(m->curmax, ((pnt_t *)n->key)->x);

  }

  mpq_add(m->cursum, m->cursum, ((pnt_t *)n->key)->y);
  mpq_set(m->curmax, ((pnt_t *)n->key)->x);

  print_mult_bin_r(n->r, m);

}


void print_mult_bin(avl_t *avl, double fac) {
  pnt_t *tp; gavl_node_t *tn;
  ctx_bin_t *m;

  if (!avl->root) return;

  m = malloc(sizeof(ctx_bin_t));
  ctx_bin_init(m);
  

  mpq_set_d(m->fac,fac);
  tn=avl->root;  while (tn->l) tn=tn->l;
  mpq_mul(m->curx, ((pnt_t *)tn->key)->x, m->fac);
  mpq_set(m->curmin, ((pnt_t *)tn->key)->x);
  mpq_set(m->curmax, ((pnt_t *)tn->key)->x);

  mpq_set_ui(m->tot, 0, 1);
  calc_tot_r(avl->root, m->tot);

  print_mult_bin_r(avl->root, m);

  if (mpq_cmp_ui(m->cursum, 0, 1) > 0)
    printfunc2(m);

  ctx_bin_free(m);
}

void print_simple_r(gavl_node_t *n, int level) {
  if (!n) return;
  print_simple_r(n->l, level+1);
  printfunc(n->key); fprintf(ofp, "\n");
  print_simple_r(n->r, level+1);
}

void print_simple2_r(gavl_node_t *n, mpq_t renorm, int level) {
  int i, j, k;
  static pnt_t *tpnt=NULL;

  if (!tpnt) {
    tpnt=malloc(sizeof(pnt_t));
    mpq_init(tpnt->x);
    mpq_init(tpnt->y);
  }

  if (!n) return;
  print_simple2_r(n->l, renorm, level+1);

  mpq_set(tpnt->x, ((pnt_t *)n->key)->x);
  mpq_set(tpnt->y, ((pnt_t *)n->key)->y);
  if (renormalize_flag && (mpq_cmp_ui(renorm, 0, 1)>0))
    mpq_div(tpnt->y, tpnt->y, renorm);

  printfunc(tpnt); fprintf(ofp, "\n");
  print_simple2_r(n->r, renorm, level+1);

}
void print_simple(avl_t *a) { 
  mpq_t renorm;
  mpq_init(renorm);
  if (renormalize_flag) {
    calc_tot_r(a->root, renorm);
    print_simple2_r(a->root, renorm, 0); 
  } else {
    print_simple_r(a->root, 0);
  }

  mpq_clear(renorm);
}

void show_help(char *s) {
  printf("%s usage:\n", s);
  printf("\t[-c]   \t cumulative distribution (default)\n");
  printf("\t[-r]   \t output raw data (usefull if you want to sum dups or sort input)\n");
  printf("\t[-b b] \t bin in b bins\n");
  printf("\t[-B B] \t multiplicative bin, x = sqrt(xwin_min, xwin_max), y = # in bin / (xwin_max - xwin_min + 1)\n");
  printf("\t[-R]   \t renormalize\n");
  printf("\t[-P]   \t output probability density function (ignored if -b not specified)\n");
  printf("\t[-D D] \t data binning technique for mult. bin. (eqns below don't account for R)\n");
  printf("\t       \t  0 - x = sqrt(B^j * B^{j+1)), y = # in bin / (B^{j+1} - B^j + 1)\n");
  printf("\t       \t  1 - x = sqrt(xwin_min, xwin_max), y = # in bin / (xwin_max - xwin_min + 1)\n");
  printf("\t       \t  2 - x = xwin_min, y = # in bin\n");
  printf("\t[-o o] \t output file (defualt stdout)\n");
  printf("\t[-i i] \t input file (default stdin)\n");
  printf("\t[-d]   \t remove duplicates (i.e. 1 1\\n1 1 -> 1 1), default sums duplicates\n");
  printf("\t[-h]   \t help (this screen)\n");
}

int main(int argc, char **argv) {
  int i, j, k, b=0, out_type=0, sum_dup=1;
  char *chp, buf[buflen], ch, *ifn=NULL, *ofn=NULL;
  avl_t *avl, *cdf; pnt_t *pnt, *tpnt;
  extern char *optarg;
  double f;

  renormalize_flag=0;
  data_technique=0;
  df_flag=0;
  

  ifp=stdin; ofp=stdout;
  while ((ch=getopt(argc, argv, "rcb:B:o:i:hdRPD:"))!=-1) switch(ch) {
    case 'c': out_type=0; break;
    case 'b': out_type=1; b=atoi(optarg); break;
    case 'r': out_type=2; break;
    case 'B': out_type=3; f=atof(optarg); break;
    case 'D': data_technique = atoi(optarg); break;
    case 'R': renormalize_flag=1; break;
    case 'P': df_flag = 1; renormalize_flag=1; break;
    case 'o': ofn=strdup(optarg); break;
    case 'i': ifn=strdup(optarg); break;
    case 'd': sum_dup=0; break;
    case 'h':
    default: show_help(argv[0]); exit(0); break;
  }

  if ((out_type==1) && (b<=0)) { printf("bins must be positive\n"); show_help(argv[0]); exit(0); }
  if ((data_technique<0) || (data_technique>2)) { printf("-D \\in [0,2]\n"); show_help(argv[0]); exit(0); }

  if (ifn) if (!(ifp=fopen(ifn, "r"))) { perror("input file"); exit(0); }
  if (ofn) {  /* poke */
    if (!(ofp=fopen(ofn, "a+"))) { perror("output file"); exit(0); }
    fclose(ofp);
  }


  avl=avl_alloc(NULL);
  avl->cmp=cmp; avl->free=freefunc; avl->print=printfunc;

  cdf=avl_alloc(NULL);
  cdf->cmp=cmp; cdf->free=freefunc; cdf->print=printfunc;

  curprec=32;
  mpf_init2(flo, curprec);

  i=0;
  while (!feof(ifp)) {

    chp=fgets(buf, buflen, ifp);

    if (!chp) continue;
    while (chp && (*chp==' ')) chp++;
    if ((!chp) || (*chp=='\n') || (*chp=='#')) continue;

    pnt=malloc(sizeof(pnt_t));
    mpq_init(pnt->x); mpq_init(pnt->y);

    for (j=0; (j<buflen) && (buf[j]==' '); j++);
    if (j==buflen) { printf("maximum buffer size exceeded, exiting\n"); exit(0); }
    chp=strchr(&(buf[j]), ' ');
    if (chp) { *chp='\0'; chp++; }

    k=4*strlen(buf);
    if (k>curprec) { curprec=k; mpf_clear(flo); mpf_init2(flo, curprec); }
    mpf_set_str(flo, buf, 10);
    mpq_set_f(pnt->x, flo);

    /* if we're in multiplicative bin mode, discard <= 0 points */
    if ((out_type==3) && (mpq_cmp_ui(pnt->x, 0, 1)<=0)) {
      freefunc(pnt); continue;
    }

    if (chp) {
      k=4*strlen(chp);
      if (k>curprec) {
        curprec=k; 
        mpf_clear(flo); 
        mpf_init2(flo, curprec);
      }
      mpf_set_str(flo, chp, 10);
      mpq_set_f(pnt->y, flo);
    } 

    k=avl_ins(avl, pnt);
    if (k<0) {
      if (sum_dup) {
        tpnt=avl_get(avl, pnt);
        mpq_add(tpnt->y, pnt->y, tpnt->y);
      }

      freefunc(pnt);

    }

  }

  if (ifn) { fclose(ifp); free(ifn); }

  if (ofn) if (!(ofp=fopen(ofn, "w"))) { perror("output file"); exit(0); }

  if (out_type==0) { print_cdf(avl); }
  else if (out_type==1) { print_bin(avl, b); }
  else if (out_type==2) { print_simple(avl); }
  else if (out_type==3) { print_mult_bin(avl, f); }
  else { printf("sanity...\n"); exit(0); }

  if (ofn) { free(ofn); fclose(ofp); }

  avl_free(avl);
  avl_free(cdf);

}
