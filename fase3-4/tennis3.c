/*****************************************************************************/
/*									     */
/*				     Tennis0.c				     */
/*									     */
/*  Programa inicial d'exemple per a les practiques 2 i 3 de ISO.	     */
/*     Es tracta del joc del tennis: es dibuixa un camp de joc rectangular    */
/*     amb una porteria a cada costat, una paleta per l'usuari, una paleta   */
/*     per l'ordinador i una pilota que va rebotant per tot arreu; l'usuari  */
/*     disposa de dues tecles per controlar la seva paleta, mentre que l'or- */
/*     dinador mou la seva automaticament (amunt i avall). Evidentment, es   */
/*     tracta d'intentar col.locar la pilota a la porteria de l'ordinador    */
/*     (porteria de la dreta), abans que l'ordinador aconseguixi col.locar   */
/*     la pilota dins la porteria de l'usuari (porteria de l'esquerra).      */
/*									     */
/*  Arguments del programa:						     */
/*     per controlar la posicio de tots els elements del joc, cal indicar    */
/*     el nom d'un fitxer de text que contindra la seguent informacio:	     */
/*		n_fil n_col m_por l_pal					     */
/*		pil_pf pil_pc pil_vf pil_vc				     */
/*		ipo_pf ipo_pc po_vf					     */
/*									     */
/*     on 'n_fil', 'n_col' son les dimensions del taulell de joc, 'm_por'    */
/*     es la mida de les dues porteries, 'l_pal' es la longitud de les dues  */
/*     paletes; 'pil_pf', 'pil_pc' es la posicio inicial (fila,columna) de   */
/*     la pilota, mentre que 'pil_vf', 'pil_vc' es la velocitat inicial;     */
/*     finalment, 'ipo_pf', 'ipo_pc' indicara la posicio del primer caracter */
/*     de la paleta de l'ordinador, mentre que la seva velocitat vertical    */
/*     ve determinada pel parametre 'po_fv'.				     */
/*									     */
/*     A mes, es podra afegir un segon argument opcional per indicar el      */
/*     retard de moviment de la pilota i la paleta de l'ordinador (en ms);   */
/*     el valor d'aquest parametre per defecte es 100 (1 decima de segon).   */
/*									     */
/*  Compilar i executar:					  	     */
/*     El programa invoca les funcions definides en 'winsuport.o', les       */
/*     quals proporcionen una interficie senzilla per a crear una finestra   */
/*     de text on es poden imprimir caracters en posicions especifiques de   */
/*     la pantalla (basada en CURSES); per tant, el programa necessita ser   */
/*     compilat amb la llibreria 'curses':				     */
/*									     */
/*	   $ gcc tennis0.c winsuport.o -o tennis0 -lcurses		     */
/*	   $ tennis0 fit_param [retard]					     */
/*									     */
/*  Codis de retorn:						  	     */
/*     El programa retorna algun dels seguents codis al SO:		     */
/*	0  ==>  funcionament normal					     */
/*	1  ==>  numero d'arguments incorrecte 				     */
/*	2  ==>  fitxer no accessible					     */
/*	3  ==>  dimensions del taulell incorrectes			     */
/*	4  ==>  parametres de la pilota incorrectes			     */
/*	5  ==>  parametres d'alguna de les paletes incorrectes		     */
/*	6  ==>  no s'ha pogut crear el camp de joc (no pot iniciar CURSES)   */
/*****************************************************************************/

// #include <stdio.h>		/* incloure definicions de funcions estandard */
// #include <stdlib.h>
// #include "winsuport.h"		/* incloure definicions de funcions propies */
// #include <pthread.h>
// #include <stdint.h>


#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "memoria.h"
#include "memoria.c"
#include "winsuport2.h"

#define MIN_FIL 7		/* definir limits de variables globals */
#define MAX_FIL 25
#define MIN_COL 10
#define MAX_COL 80
#define MIN_PAL 3
#define MIN_VEL -1.0
#define MAX_VEL 1.0
#define MAX_PALETA 9
#define MAX_THREAD 2

typedef struct{
  int ipo_pf, ipo_pc;      	/* posicio del la paleta de l'ordinador */
  float po_pf;	/* pos. vertical de la paleta de l'ordinador, en valor real */
  float v_pal;			/* velocitat de la paleta del programa */
} Paleta;

/* variables globals */
int n_fil, n_col, m_por;	/* dimensions del taulell i porteries */
int l_pal;			/* longitud de les paletes */

int ipu_pf, ipu_pc;      	/* posicio del la paleta d'usuari */

int ipil_pf, ipil_pc;		/* posicio de la pilota, en valor enter */
float pil_pf, pil_pc;		/* posicio de la pilota, en valor real */
float pil_vf, pil_vc;		/* velocitat de la pilota, en valor real*/

int retard;		/* valor del retard de moviment, en mil.lisegons */

pthread_mutex_t mutex_pantalla= PTHREAD_MUTEX_INITIALIZER;   /* crea un sem. Global*/
pthread_mutex_t mutex_vglobals= PTHREAD_MUTEX_INITIALIZER;   /* crea un sem. Global*/
Paleta paletes[MAX_PALETA];
pthread_t tid[MAX_THREAD];
int numPaletes=0;
int numMovimentsMax;

int id_win, id_n_moviments, id_tecla, id_cont;
int *p_win, *p_n_moviments, *p_tecla, *p_cont;
pid_t tpid[MAX_PALETA];    /* taula d'identificadors dels processos fill */

/* funcio per realitzar la carrega dels parametres de joc emmagatzemats */
/* dins un fitxer de text, el nom del qual es passa per referencia en   */
/* 'nom_fit'; si es detecta algun problema, la funcio avorta l'execucio */
/* enviant un missatge per la sortida d'error i retornant el codi per-	*/
/* tinent al SO (segons comentaris del principi del programa).		*/
void carrega_parametres(const char *nom_fit){
  FILE *fit;

  fit = fopen(nom_fit,"rt");		/* intenta obrir fitxer */
  if (fit == NULL){
    fprintf(stderr,"No s'ha pogut obrir el fitxer \'%s\'\n",nom_fit);
  	exit(2);
  }

  if (!feof(fit))
    fscanf(fit,"%d %d %d %d\n",&n_fil,&n_col,&m_por,&l_pal);
  if ((n_fil < MIN_FIL) || (n_fil > MAX_FIL) || (n_col < MIN_COL) || (n_col > MAX_COL) || (m_por < 0) || (m_por > n_fil-3) || (l_pal < MIN_PAL) || (l_pal > n_fil-3)){
  	fprintf(stderr,"Error: dimensions del camp de joc incorrectes:\n");
  	fprintf(stderr,"\t%d =< n_fil (%d) =< %d\n",MIN_FIL,n_fil,MAX_FIL);
  	fprintf(stderr,"\t%d =< n_col (%d) =< %d\n",MIN_COL,n_col,MAX_COL);
  	fprintf(stderr,"\t0 =< m_por (%d) =< n_fil-3 (%d)\n",m_por,(n_fil-3));
  	fprintf(stderr,"\t%d =< l_pal (%d) =< n_fil-3 (%d)\n",MIN_PAL,l_pal,(n_fil-3));
  	fclose(fit);
  	exit(3);
  }

  if (!feof(fit))
    fscanf(fit,"%d %d %f %f\n",&ipil_pf,&ipil_pc,&pil_vf,&pil_vc);
  if ((ipil_pf < 1) || (ipil_pf > n_fil-3) || (ipil_pc < 1) || (ipil_pc > n_col-2) || (pil_vf < MIN_VEL) || (pil_vf > MAX_VEL) || (pil_vc < MIN_VEL) || (pil_vc > MAX_VEL)){
  	fprintf(stderr,"Error: parametre pilota incorrectes:\n");
  	fprintf(stderr,"\t1 =< ipil_pf (%d) =< n_fil-3 (%d)\n",ipil_pf,(n_fil-3));
  	fprintf(stderr,"\t1 =< ipil_pc (%d) =< n_col-2 (%d)\n",ipil_pc,(n_col-2));
  	fprintf(stderr,"\t%.1f =< pil_vf (%.1f) =< %.1f\n",MIN_VEL,pil_vf,MAX_VEL);
  	fprintf(stderr,"\t%.1f =< pil_vc (%.1f) =< %.1f\n",MIN_VEL,pil_vc,MAX_VEL);
  	fclose(fit);
  	exit(4);
  }

  while(!feof(fit) && (numPaletes<MAX_PALETA)){
    fscanf(fit,"%d %d %f\n", &paletes[numPaletes].ipo_pf, &paletes[numPaletes].ipo_pc, &paletes[numPaletes].v_pal);

    if ((paletes[numPaletes].ipo_pf < 1) || (paletes[numPaletes].ipo_pf+l_pal > n_fil-2) || (paletes[numPaletes].ipo_pc < 5) || (paletes[numPaletes].ipo_pc > n_col-2) || (paletes[numPaletes].v_pal < MIN_VEL) || (paletes[numPaletes].v_pal > MAX_VEL)){
    	fprintf(stderr,"Error: parametres paleta ordinador incorrectes:\n");
    	fprintf(stderr,"\t1 =< ipo_pf (%d) =< n_fil-l_pal-3 (%d)\n",paletes[numPaletes].ipo_pf,(n_fil-l_pal-3));
    	fprintf(stderr,"\t5 =< ipo_pc (%d) =< n_col-2 (%d)\n",paletes[numPaletes].ipo_pc,(n_col-2));
    	fprintf(stderr,"\t%.1f =< v_pal (%.1f) =< %.1f\n",MIN_VEL,paletes[numPaletes].v_pal,MAX_VEL);

      fclose(fit);
      exit(5);
    }else
      numPaletes++;
  }

  fclose(fit);			/* fitxer carregat: tot OK! */
}

/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
int inicialitza_joc(void){
  int i, j, i_port, f_port, retwin;
  char strin[51];
  retwin = win_ini(&n_fil,&n_col,'+',INVERS);   /* intenta crear taulell */

  if(retwin < 0){       /* si no pot crear l'entorn de joc amb les curses */
    fprintf(stderr,"Error en la creacio del taulell de joc:\t");
    switch (retwin){
      case -1: fprintf(stderr,"camp de joc ja creat!\n");
               break;
      case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n");
 		           break;
      case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n");
               break;
      case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n");
               break;
     }
     return(retwin);
  }

  id_win = ini_mem(retwin);  /* crear zona mem. compartida */
  p_win = map_mem(id_win);   //obtenir adres. de mem. compartida

  id_n_moviments = ini_mem(sizeof(int));
  p_n_moviments = map_mem(id_n_moviments);

  id_tecla = ini_mem(sizeof(char));
  p_tecla = map_mem(id_tecla);
  *p_tecla = 0;

  id_cont = ini_mem(sizeof(int));
  p_cont = map_mem(id_cont);
  *p_cont = -1;

  win_set(p_win, n_fil, n_col);

  i_port = n_fil/2 - m_por/2;	    /* crea els forats de la porteria */
  if (n_fil%2 == 0) i_port--;
  if (i_port == 0) i_port=1;
  f_port = i_port + m_por -1;
  for (i = i_port; i <= f_port; i++){
    win_escricar(i,0,' ',NO_INV);
    win_escricar(i,n_col-1,' ',NO_INV);
  }

  ipu_pf = n_fil/2; ipu_pc = 3;		/* inicialitzar pos. paletes */
  if (ipu_pf+l_pal >= n_fil-3) ipu_pf = 1;

  for (i=0; i< l_pal; i++){	    /* dibuixar paleta inicialment */
    win_escricar(ipu_pf +i, ipu_pc, '0',INVERS);
  }
  for (j=0; j<numPaletes; j++) {
    for (i=0; i< l_pal; i++){
  	  win_escricar(paletes[j].ipo_pf +i, paletes[j].ipo_pc, (j+1+'0'),INVERS);
    }
    paletes[j].po_pf = paletes[j].ipo_pf;		/* fixar valor real paleta ordinador */
  }

  pil_pf = ipil_pf; pil_pc = ipil_pc;	/* fixar valor real posicio pilota */
  win_escricar(ipil_pf, ipil_pc, '.',INVERS);	/* dibuix inicial pilota */

  sprintf(strin,"Tecles: \'%c\'-> amunt, \'%c\'-> avall, RETURN-> sortir.", TEC_AMUNT, TEC_AVALL);
  win_escristr(strin);
  return(0);
}

/* funcio per moure la pilota; retorna un valor amb alguna d'aquestes	*/
/* possibilitats:							*/
/*	-1 ==> la pilota no ha sortit del taulell			*/
/*	 0 ==> la pilota ha sortit per la porteria esquerra		*/
/*	>0 ==> la pilota ha sortit per la porteria dreta		*/
void * moure_pilota(void * cap){
  int f_h, c_h;
  char rh,rv,rd;

  do{
    win_retard(retard);

    f_h = pil_pf + pil_vf;		/* posicio hipotetica de la pilota */
    c_h = pil_pc + pil_vc;

    rh = rv = rd = ' ';
    if ((f_h != ipil_pf) || (c_h != ipil_pc)){		/* si posicio hipotetica no coincideix amb la pos. actual */
      if (f_h != ipil_pf){		/* provar rebot vertical */
        pthread_mutex_lock(&mutex_pantalla);
    	  rv = win_quincar(f_h,ipil_pc);	/* veure si hi ha algun obstacle */
        pthread_mutex_unlock(&mutex_pantalla);
  	    if (rv != ' '){			/* si no hi ha res */
  	      pil_vf = -pil_vf;		/* canvia velocitat vertical */
  	      f_h = pil_pf+pil_vf;	/* actualitza posicio hipotetica */
  	    }
      }
      if (c_h != ipil_pc){	/* provar rebot horitzontal */
        pthread_mutex_lock(&mutex_pantalla);
        rh = win_quincar(ipil_pf,c_h);	/* veure si hi ha algun obstacle */
        pthread_mutex_unlock(&mutex_pantalla);
  	    if (rh != ' '){			/* si no hi ha res */
          pil_vc = -pil_vc;		/* canvia velocitat horitzontal */
  	      c_h = pil_pc+pil_vc;	/* actualitza posicio hipotetica */
  	    }
      }
      if ((f_h != ipil_pf) && (c_h != ipil_pc)){	/* provar rebot diagonal */
        pthread_mutex_lock(&mutex_pantalla);
      	rd = win_quincar(f_h,c_h);
        pthread_mutex_unlock(&mutex_pantalla);
  	    if (rd != ' '){				/* si no hi ha obstacle */
          pil_vf = -pil_vf; pil_vc = -pil_vc;	/* canvia velocitats */
  	      f_h = pil_pf+pil_vf;
  	      c_h = pil_pc+pil_vc;		/* actualitza posicio entera */
  	    }
      }
      pthread_mutex_lock(&mutex_pantalla);
      if (win_quincar(f_h,c_h) == ' '){/* verificar posicio definitiva *//* si no hi ha obstacle */
        win_escricar(ipil_pf,ipil_pc,' ',NO_INV);	/* esborra pilota */
        pthread_mutex_unlock(&mutex_pantalla);
      	pil_pf += pil_vf; pil_pc += pil_vc;
      	ipil_pf = f_h; ipil_pc = c_h;		/* actualitza posicio actual */
      	if ((ipil_pc > 0) && (ipil_pc <= n_col)){	/* si no surt */
          pthread_mutex_lock(&mutex_pantalla);
      		win_escricar(ipil_pf,ipil_pc,'.',INVERS); /* imprimeix pilota */
          pthread_mutex_unlock(&mutex_pantalla);
      	}else
      		*p_cont = ipil_pc;	/* codi de finalitzacio de partida */
      }else
        pthread_mutex_unlock(&mutex_pantalla);
    }else{
      pil_pf += pil_vf; pil_pc += pil_vc;
    }
    pthread_mutex_lock(&mutex_vglobals);
    if(*p_n_moviments<=0){
      pthread_mutex_unlock(&mutex_vglobals);
      pthread_exit(0);
    }else
      pthread_mutex_unlock(&mutex_vglobals);
  } while ((*p_tecla != TEC_RETURN) && (*p_cont==-1));
  pthread_exit(0);
}

/* funcio per moure la paleta de l'usuari en funcio de la tecla premuda */
void * mou_paleta_usuari(void * cap){
  do{
    win_retard(retard);
    pthread_mutex_lock(&mutex_vglobals);
    *p_tecla = win_gettec();
    pthread_mutex_lock(&mutex_pantalla);
    if ((*p_tecla == TEC_AVALL) && (win_quincar(ipu_pf+l_pal,ipu_pc) == ' ')){
      win_escricar(ipu_pf,ipu_pc,' ',NO_INV);	   /* esborra primer bloc */
      ipu_pf++;					   /* actualitza posicio */
      win_escricar(ipu_pf+l_pal-1,ipu_pc,'0',INVERS); /* impri. ultim bloc */
    }
    if ((*p_tecla == TEC_AMUNT) && (win_quincar(ipu_pf-1,ipu_pc) == ' ')){
      win_escricar(ipu_pf+l_pal-1,ipu_pc,' ',NO_INV); /* esborra ultim bloc */
      ipu_pf--;					    /* actualitza posicio */
      win_escricar(ipu_pf,ipu_pc,'0',INVERS);	    /* imprimeix primer bloc */
    }
    pthread_mutex_unlock(&mutex_pantalla);
    if(*p_n_moviments>0){
      if((*p_tecla == TEC_AMUNT) || (*p_tecla == TEC_AVALL))
        *p_n_moviments=*p_n_moviments-1;
      pthread_mutex_unlock(&mutex_vglobals);
    }else{
      pthread_mutex_unlock(&mutex_vglobals);
      pthread_exit(0);
    }
  } while ((*p_tecla != TEC_RETURN) && (*p_cont==-1));
  pthread_exit(0);
}

/* funcio per moure la paleta de l'ordinador autonomament, en funcio de la */
/* velocitat de la paleta (variable global v_pal) */
// void * mou_paleta_ordinador(void *index){
//   int f_h;
//   int index_th = (intptr_t) index;
//  /* char rh,rv,rd; */
//   do{
//     win_retard(retard);
//     f_h = paletes[index_th].po_pf + paletes[index_th].v_pal;		/* posicio hipotetica de la paleta */
//     if (f_h != paletes[index_th].ipo_pf){	/* si pos. hipotetica no coincideix amb pos. actual */
//       if (paletes[index_th].v_pal > 0.0){			/* verificar moviment cap avall */
//         pthread_mutex_lock(&mutex_pantalla);
//         if (win_quincar(f_h+l_pal-1,paletes[index_th].ipo_pc) == ' '){   /* si no hi ha obstacle */
//   	      win_escricar(paletes[index_th].ipo_pf,paletes[index_th].ipo_pc,' ',NO_INV);      /* esborra primer bloc */
//           // pthread_mutex_unlock(&mutex_pantalla);
//   	      paletes[index_th].po_pf += paletes[index_th].v_pal; paletes[index_th].ipo_pf = paletes[index_th].po_pf;		/* actualitza posicio */
//   	      // pthread_mutex_lock(&mutex_pantalla);
//           win_escricar(paletes[index_th].ipo_pf+l_pal-1,paletes[index_th].ipo_pc,(index_th+1+'0'),INVERS); /* impr. ultim bloc */
//   	      pthread_mutex_unlock(&mutex_pantalla);
//         }else{		/* si hi ha obstacle, canvia el sentit del moviment */
//           pthread_mutex_unlock(&mutex_pantalla);
//   	      paletes[index_th].v_pal = -paletes[index_th].v_pal;
//         }
//       }else{			/* verificar moviment cap amunt */
//         pthread_mutex_lock(&mutex_pantalla);
//         if (win_quincar(f_h,paletes[index_th].ipo_pc) == ' '){        /* si no hi ha obstacle */
//       	  win_escricar(paletes[index_th].ipo_pf+l_pal-1,paletes[index_th].ipo_pc,' ',NO_INV); /* esbo. ultim bloc */
//       	  // pthread_mutex_unlock(&mutex_pantalla);
//           paletes[index_th].po_pf += paletes[index_th].v_pal; paletes[index_th].ipo_pf = paletes[index_th].po_pf;		/* actualitza posicio */
//       	  // pthread_mutex_lock(&mutex_pantalla);
//           win_escricar(paletes[index_th].ipo_pf,paletes[index_th].ipo_pc,(index_th+1+'0'),INVERS);	/* impr. primer bloc */
//       	  pthread_mutex_unlock(&mutex_pantalla);
//         }else{		/* si hi ha obstacle, canvia el sentit del moviment */
//           pthread_mutex_unlock(&mutex_pantalla);
//       	  paletes[index_th].v_pal = -paletes[index_th].v_pal;
//         }
//       }
//     }else paletes[index_th].po_pf += paletes[index_th].v_pal; 	/* actualitza posicio vertical real de la paleta */
//     pthread_mutex_lock(&mutex_vglobals);
//     if(numMoviments>0){
//       numMoviments--;
//       pthread_mutex_unlock(&mutex_vglobals);
//     }else{
//       pthread_mutex_unlock(&mutex_vglobals);
//       pthread_exit(0);
//     }
//
//   } while ((tecla != TEC_RETURN) && (cont==-1));
//   pthread_exit(0);
// }

/* programa principal				    */
int main(int n_args, const char *ll_args[]){
  int i, min, seg, n;
  time_t inici, final;
  char buffer[100];

  if ((n_args != 2) && (n_args !=3) && (n_args !=4)){
    fprintf(stderr,"Comanda: tennis2 fit_param [numMoviments] [retard]\n");
  	exit(1);
  }
  carrega_parametres(ll_args[1]);

  if (n_args == 4){
    numMovimentsMax=atoi(ll_args[2]);
    retard=atoi(ll_args[3]);
  }else if (n_args == 3){
    numMovimentsMax=atoi(ll_args[2]);
    retard = 100;
  }

  if (inicialitza_joc() !=0)    /* intenta crear el taulell de joc */
    exit(4);   /* aborta si hi ha algun problema amb taulell */

  *p_n_moviments = numMovimentsMax;

  pthread_mutex_init(&mutex_vglobals, NULL);		/* inicialitza el semafor */
  pthread_mutex_init(&mutex_pantalla, NULL);    /* inicialitza el semafor */
  pthread_create(&tid[0],NULL,mou_paleta_usuari, NULL);
  pthread_create(&tid[1],NULL,moure_pilota,NULL);

  char str_n_fil[3], str_n_col[3], str_l_pal[3], str_ipo_pf[4], str_ipo_pc[4], str_po_pf[4], str_v_pal[4], str_id_win[20], str_id_n_moviments[20], str_id_tecla[20], str_id_cont[20], str_index[1];
  sprintf(str_n_fil, "%i", n_fil);
  sprintf(str_n_col, "%i", n_col);
  sprintf(str_l_pal, "%i", l_pal);
  sprintf(str_id_win, "%i", id_win);
  sprintf(str_id_n_moviments, "%i", id_n_moviments);
  sprintf(str_id_tecla, "%i", id_tecla);
  sprintf(str_id_cont, "%i", id_cont);

  n = 0;
  for ( i = 0; i < numPaletes; i++){
    tpid[n] = fork();   /* crea un nou proces */
    if (tpid[n] == (pid_t) 0){   /* branca del fill */
      sprintf(str_ipo_pf, "%i", paletes[i].ipo_pf);
      sprintf(str_ipo_pc, "%i", paletes[i].ipo_pc);
      sprintf(str_po_pf, "%f", paletes[i].po_pf);
      sprintf(str_v_pal, "%f", paletes[i].v_pal);
      sprintf(str_index, "%i", i+1);
      sprintf(str_id_cont, "%i", id_cont);

      execlp("./pal_ord3", "pal_ord3", str_n_fil, str_n_col, str_l_pal, str_ipo_pf, str_ipo_pc, str_po_pf, str_v_pal, str_id_win, str_id_n_moviments, str_id_tecla, str_id_cont, str_index, (char *)0);
      fprintf(stderr,"error: no puc executar el process fill \'pal_ord3\'\n");
      exit(0);
    }else if (tpid[n] > 0) n++;    /* branca del pare */
  }

  inici=time(NULL);
  while((*p_cont==-1)&&(*p_n_moviments>0)&&(*p_tecla != TEC_RETURN)){
    final=time(NULL);
    seg=difftime(final,inici);
    min=seg/60;
    seg=seg%60;
    pthread_mutex_lock(&mutex_vglobals);
    sprintf(buffer,"Fets: %d Restants: %d Temps: %2d:%2d", (numMovimentsMax-*p_n_moviments),*p_n_moviments, min, seg);
    pthread_mutex_unlock(&mutex_vglobals);
    pthread_mutex_lock(&mutex_pantalla);
    win_escristr(buffer);
    pthread_mutex_unlock(&mutex_pantalla);
    win_update();
    win_retard(retard);
  }

  for(i=0; i<(MAX_THREAD); i++){
    pthread_join(tid[i], NULL);
  }
  pthread_mutex_destroy(&mutex_pantalla);
  pthread_mutex_destroy(&mutex_vglobals);
  win_fi();

  if (*p_tecla == TEC_RETURN) printf("S'ha aturat el joc amb la tecla RETURN!\n");
  else{
    if (*p_cont == 0) printf("Ha guanyat l'ordinador!\n");
    else if (*p_cont > 0) printf("Ha guanyat l'usuari!\n");
    else printf("S'han acabat els moviments!\n");
  }

  elim_mem(id_win);
  elim_mem(id_n_moviments);
  elim_mem(id_tecla);
  elim_mem(id_cont);
  return(0);
}
