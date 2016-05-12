#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "memoria.h"
#include "memoria.c"
#include "winsuport2.h"

typedef struct{
  int ipo_pf, ipo_pc;       /* posicio del la paleta de l'ordinador */
  float po_pf;  /* pos. vertical de la paleta de l'ordinador, en valor real */
  float v_pal;      /* velocitat de la paleta del programa */
} Paleta;

int n_fil, n_col, l_pal, id_n_moviments, id_tecla, id_cont;
Paleta paleta;
int *p_win, *p_n_moviments, *p_tecla, *p_cont;

int main(int n_args, char *ll_args[]){
  int f_h;

  n_fil = atoi(ll_args[1]);
  n_col = atoi(ll_args[2]);
  l_pal = atoi(ll_args[3]);

  paleta.ipo_pf = atoi(ll_args[4]);
  paleta.ipo_pc = atoi(ll_args[5]);
  paleta.po_pf = atof(ll_args[6]);
  paleta.v_pal = atof(ll_args[7]);

  p_win = map_mem(atoi(ll_args[8]));
  p_n_moviments = map_mem(atoi(ll_args[9]));
  p_tecla = map_mem(atoi(ll_args[10]));
  p_cont = map_mem(atoi(ll_args[11]));

  win_set(p_win, n_fil, n_col);

  do{
    f_h = paleta.po_pf + paleta.v_pal;    /* posicio hipotetica de la paleta */
    if (f_h != paleta.ipo_pf){ /* si pos. hipotetica no coincideix amb pos. actual */
      if (paleta.v_pal > 0.0){     /* verificar moviment cap avall */
        if (win_quincar(f_h+l_pal-1,paleta.ipo_pc) == ' '){   /* si no hi ha obstacle */
          win_escricar(paleta.ipo_pf,paleta.ipo_pc,' ',NO_INV);     /*  esborra primer bloc*/
          paleta.po_pf += paleta.v_pal; paleta.ipo_pf = paleta.po_pf;   /* actualitza posicio */
          win_escricar(paleta.ipo_pf+l_pal-1,paleta.ipo_pc,ll_args[12][0],INVERS); /* impr. ultim bloc */
        }else{    /* si hi ha obstacle, canvia el sentit del moviment */
          paleta.v_pal = -paleta.v_pal;
        }
      }else{      /* verificar moviment cap amunt */
        if (win_quincar(f_h,paleta.ipo_pc) == ' '){        /* si no hi ha obstacle */
          win_escricar(paleta.ipo_pf+l_pal-1,paleta.ipo_pc,' ',NO_INV); /* esbo. ultim bloc */
          paleta.po_pf += paleta.v_pal; paleta.ipo_pf = paleta.po_pf;   /* actualitza posicio */
          win_escricar(paleta.ipo_pf,paleta.ipo_pc,ll_args[12][0],INVERS);  /* impr. primer bloc */
        }else{    /* si hi ha obstacle, canvia el sentit del moviment */
          paleta.v_pal = -paleta.v_pal;
        }
      }
    }else paleta.po_pf += paleta.v_pal;   /* actualitza posicio vertical real de la paleta */
    if(*p_n_moviments>0){
      *p_n_moviments=*p_n_moviments-1;
    }

  } while ((*p_tecla != TEC_RETURN) && (*p_cont==-1) && (*p_n_moviments>0));
}
