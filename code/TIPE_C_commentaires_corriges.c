#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>


#define MAX_ROWS 380 // Nombre maximum de lignes dans le fichier CSV
#define MAX_COLS 65  // Nombre maximum de colonnes dans le fichier CSV
#define MAX_LINE_LENGTH 65
//on représente une victoire par un 2,un match nul par un 1 et une défaite par un 0 dans la colonne [4]
// structures

struct Noeud 
{
    int* indexDonnee;   //tableau contenant les indices des données prises en compte
    int cardDonnee;     //taille de index_donnee
    float seuil;        //seuil permettra de parcourir l'abre créé
    int* indParams;     //tableau contenant les indices des paramétres pris en compte
    int cardParam;      //taille de indParams
    int paramSplit;     //indices du paramétre dans IndParam sur lequel on sépare pour aller à fg ou fd
    int hauteur;        //hauteur permet de connaitre où on se trouve dans l'abre
    bool feuille;       //permet de savoir si on se trouve sur une feuille lors d'un parcours
    struct Noeud* gauche;
    struct Noeud* droit;
    struct Noeud* millieu;
};
typedef struct Noeud Noeud;

struct AbrDecision 
{
    int profondeurMax;        //permet de connaitre la profondeur maximum de parcours
    int nbSeparationMin;      //permet d'éviter d'avoir des arbres de taille trop faible
    int nbFeuilleMin;         //pour éviter les abres de trop faible dispersion
    int* oobIndices;          //tableau des indices hors boite pour cet arbre
    int card;                 //cardinal de oob
    Noeud* racine;
};
typedef struct AbrDecision AbrDecision;

struct Foretalea
{
    int nbEstimateur;       //nombre d'arbres souhaités
    int profMax;            //profondeur maximale par arbre
    int nbSeparationMin;  
    int nbFeuilleMin;
    int nbParamParArbre;    //nombre de paramètre par arbre
    int nbParam;            //ombre de paramètre total
    float fiabilite;        //fiabilité de la forét
    AbrDecision** forest;
};
typedef struct Foretalea Foretalea;

//fonctions pour afficher (utile pour vérifier et débugguer)
void affiche(Noeud* arbre){
    if(arbre->cardDonnee== 0)
    {
        printf("! Warning Manque de donnée !\n");
    }
    else
    {
        printf("Hauteur %d ",arbre->hauteur);
        printf("Separatrice : %f ",arbre->seuil);
        printf("Card donnée : %d ",arbre->cardDonnee);
        if( arbre->feuille)
        {
            printf("feuille ");
        }
        else
        {
            printf("Paramsplit : %d ",arbre->indParams[arbre->paramSplit]);
        }
        printf("Index: [");
        for (int i = 0; i < arbre->cardDonnee ; i++)
        {
            printf("%d,",arbre->indexDonnee[i]);
        }
        printf("] Param [");
        for (int i = 0; i<arbre->cardParam;i++)
        {
            printf("%d,",arbre->indParams[i]);
        }
        printf("]\n");
        if (arbre->feuille)
        {
        }
        else
        {
        printf("<-\n");   
        affiche(arbre->gauche);
        printf("| \n");
        affiche(arbre->millieu);
        printf("->\n");
        affiche(arbre->droit);
        }
    }
}

void affiche_arbre(AbrDecision* arbre)
{
    printf(" bootstrap : [");
    for(int i = 0 ;i<arbre->card;i++)
    {
        printf(" %d,",arbre->oobIndices[i]);
    }
    printf("] \n");
    affiche(arbre->racine);
}

Noeud* creerNoeud(int* index, int cardIndex, float separatrice, int* IndParams, int cardParam,int hauteur)
{
    Noeud* a = malloc(sizeof(Noeud));
    a->gauche = malloc(sizeof(Noeud));
    assert(a->gauche!= NULL);
    a->droit = malloc(sizeof(Noeud));
    assert(a->droit!= NULL);
    a->millieu = malloc(sizeof(Noeud));
    a->indexDonnee = index;
    a->cardDonnee = cardIndex;
    a->seuil = separatrice;
    a->indParams = IndParams;
    a->cardParam = cardParam;
    a->hauteur = hauteur;
    a->paramSplit = -1;
    a->feuille = true;
    return a;
}

float entropy(int* indices, int card,float data[MAX_ROWS][MAX_COLS]) 
{   
   //indices contient les indices de tout les éléments considérés et card en est le nombre
   float vict = 0.0;
   for (int i = 0; i<card ;i++)
   {
    if (data[indices[i]][4]== 2.0)
    {
        vict+= 1.0/card;
    }
   }
   if (vict == 0.0 || vict == 1)
   {
    return 0.0;
   }
   else
   {
    return -vict*log10f(vict)-(1.0-vict)*log10f(1.0-vict);
   }
}
float* informationGain(int* indices,int card,int param,float data[MAX_ROWS][MAX_COLS])
{//indices contient les indices dans table de l'ensemble qu'on considère, card son cardinal, param l'indice du paramètre par rapport 
 //auquel on considére le split
    float HS = entropy(indices,card,data); //entropie du parent
    float separatrice = 0;
    for (int i =0; i<card; i++) //on calcule la moyenne du paramètre choisi pour la séparation
    {     
        separatrice += (float)data[i][param];
    }
    separatrice = separatrice/(float)card;
    //permet de connaitre quel élément de indices ira à gauche et lequel à droite
    int *supsou = malloc(sizeof(int)*card); 
    assert(supsou);
    int moins = 0;
    int plus = 0;
    int egal = 0;
    //on sépare l'ensemble considéré entre les valeurs supérieures et inférieures à la moyenne
    for (int i = 0 ; i<card;i++){ 
        if ((float)data[indices[i]][param]<separatrice)
        {
            supsou[i] = -1;     //-1 pour gauche
            moins++;
        }
        else if((float)data[indices[i]][param]>separatrice)
        {
            supsou[i] = 1; //1 pour droite
            plus++;
        }
        else
        {
            supsou[i] = 0;  //0 pour au milieu
            egal++;
        }
    }
    int* ind_moins = malloc(sizeof(int)*moins);
    int* ind_plus = malloc(sizeof(int)*plus);
    int* ind_egal = malloc(sizeof(int)*egal);
    moins =0;
    plus = 0;
    egal = 0;
    for (int i = 0;i<card;i++)
    {
        if(supsou[i]==-1)
        {
            ind_moins[moins] = indices[i];
            moins++;
        }
        else if(supsou[i] == 1)
        {
            ind_plus[plus] = indices[i];
            plus++;
        }
        else
        {
            ind_egal[egal] = indices[i];
        }
    }
    free(supsou);
    //calcul du gain d'information
    float res = HS - ((float)moins/(float)card)*entropy(ind_moins,moins,data) -((float)plus/(float)card)*entropy(ind_plus,plus,data) -entropy(ind_egal,egal,data);
     //on stocke toutes les informations intéressantes dans un tableau pour ne pas réeffectuer le calcul dans la prochaine fonction
    float* tabres = malloc(sizeof(float)*(moins+plus+egal+5));
    assert(tabres);
    tabres[0] = res;
    tabres[1] = (float)moins;
    tabres[2] = (float)plus;
    tabres[3] = (float)egal;
    tabres[4] = separatrice;
    for (int i = 0;i<moins;i++)
    {
        tabres[i+5] = (float)ind_moins[i];
    }
    for (int i = 0;i<plus;i++)
    {
        tabres[i+5+moins] = (float)ind_plus[i];
    }
    for(int i=0;i<egal;i++)
    {
        tabres[i+5+moins+plus] = (float)ind_egal[i];
    }
    return tabres; 
}

Noeud* trouverPointdeSplit(int params[],int indices[], int card_donnee,int cardParam,int hauteur,float data[MAX_ROWS][MAX_COLS])
{ //params contient l'ensemble des indices considérés pour créer un arbre donné, indices l'ensemble des indices que l'on considère
    float Gain_max = 0.0;                                                   
    int indgainmax = 0;
    float* ind_gain = malloc(sizeof(float)*(card_donnee+3));
    //on va donner à chaque enfant les mêmes paramètres excepté celui pour split
    int* Paramsenfant = malloc(sizeof(int)*(cardParam-1) ); 
    assert(Paramsenfant);
    assert(ind_gain);
    //on cherche le paramètre tel que le gain d'information soit maximal
    for (int i = 0; i<cardParam; i++)
    { 
        float* temp = informationGain(indices,card_donnee,params[i],data);
        if (Gain_max<temp[0])
        {
            Gain_max = temp[0];
            ind_gain = temp;
            indgainmax = i;
        }
    }
    //récupération des informations
    int moins = (int)ind_gain[1];
    int plus = (int)ind_gain[2];
    int egal = (int)ind_gain[3];
    int* ind_moins = malloc(sizeof(int)*moins);
    int* ind_plus = malloc(sizeof(int)*plus);
    int* ind_egal = malloc(sizeof(int)*egal);
    for (int i = 0; i<moins;i++)
    {      
        ind_moins[i] = (int)ind_gain[i+5];
    }
    for (int i = 0; i<plus;i++)
    {
        ind_plus[i] = (int)ind_gain[i+5+moins];
    }
    for (int i = 0;i<egal;i++)
    {
        ind_egal[i] = (int)ind_egal[i+5+moins+plus];
    }
    float separatrice = ind_gain[4];
    Noeud* parent = creerNoeud(indices,card_donnee,separatrice,params,cardParam,hauteur);
    parent->paramSplit = indgainmax;
    int compteur = 0;
    for (int i = 0; i<cardParam;i++)
    {
        if(i != indgainmax){
            Paramsenfant[ compteur] = params[i];
            compteur++;
        }
    }
    Noeud* fg = creerNoeud(ind_moins,moins,0.0,Paramsenfant,cardParam-1,hauteur+1);
    Noeud* fd = creerNoeud(ind_plus,plus,0.0,Paramsenfant,cardParam-1,hauteur+1);
    Noeud* fm = creerNoeud(ind_egal,egal,0.0,Paramsenfant,cardParam-1,hauteur+1);
    parent ->droit = fd;
    parent ->gauche = fg;
    parent->millieu = fm;
    parent->feuille = false;
    return parent;
}

Noeud* creaParcourNoeud(Noeud* racine, int prof_max,float data[MAX_ROWS][MAX_COLS])
{
    //si la profondeur max est atteinte ou s'il n'y a plus assez de données ou de paramètres on arréte de séparer
    if (racine->hauteur == prof_max || racine->cardDonnee == 1||racine->cardDonnee==0||racine->cardParam==0) 
    {
        return racine;
    }
    else 
    {   //sinon on a affecté à fils droit et fils gauche la valeur de leur split (split renvoie le Noeud dont les fils sont les deux ensembles obtenus)
        Noeud* fils = trouverPointdeSplit(racine->indParams,racine->indexDonnee, racine->cardDonnee, racine->cardParam, racine->hauteur,data);
        racine->seuil = fils->seuil;
        racine->gauche = fils->gauche;
        racine->droit = fils->droit;
        racine->millieu = fils->millieu;
        //on applique ensuite récursivement le principe de création d'un arbre
        racine->gauche = creaParcourNoeud(racine->gauche, prof_max,data); 
        racine->droit = creaParcourNoeud(racine->droit,prof_max,data);
        racine->millieu = creaParcourNoeud(racine->millieu,prof_max,data);
        racine->feuille = false;
        racine->paramSplit = fils->paramSplit;
        free(fils);
        return racine;
    }
}

AbrDecision* creer_arbre(int* ensparam,int* indices,int*boots,int cardParam,int cardIndex,int cardboots,int profondeur_max,float data[MAX_ROWS][MAX_COLS])
{
    Noeud* arbre = creerNoeud(indices,cardIndex,0,ensparam,cardParam,0);
    arbre = creaParcourNoeud(arbre,profondeur_max,data);
    AbrDecision* abr = malloc(sizeof(AbrDecision));
    abr->racine = malloc(sizeof(Noeud*));
    assert(abr->racine!= NULL);
    abr->nbFeuilleMin= 5;
    abr->nbSeparationMin = 5;
    abr->profondeurMax = profondeur_max;
    abr-> racine = arbre ;
    abr->oobIndices = boots;
    abr->card = cardboots;
    return abr; 
}

float prediction(Noeud* abr,float Indices_test[],float data[MAX_ROWS][MAX_COLS])
{
    float tab[3] = {0.,0.,0.};
    int max = 0;
    if (abr->feuille)
    {   //si on est sur une feuille on renvoit la classe majoritaire
        for(int i = 0;i<abr->cardDonnee;i++)
        {
            int predict = (int)data[abr->indexDonnee[i]][4];
            tab[predict] += 1.;
        }
        for(int i = 0;i<3;i++)
        {
            if (tab[max] <tab[i])
            {
                max = i;
            }
        }
        return  (float)max;
    }
    else
    {
        if(Indices_test[abr->indParams[abr->paramSplit]] < abr->seuil)
        {
            if(abr->gauche->cardDonnee==0)
            {
                //s'il n'y as pas assez de données on renvoit la classe majoritaire du noeud précédent
                for(int i = 0;i<abr->cardDonnee;i++)
                {
                    int predict = (int)data[abr->indexDonnee[i]][4];
                    tab[predict] += 1.;
                }
                for(int i = 0;i<3;i++)
                {
                    if (tab[max] <tab[i])
                    {
                        max = i;
                    }
                }
                return  (float)max;return data[abr->indexDonnee[0]][4];
                }
            else
            {
                return(prediction(abr->gauche,Indices_test,data));
            }
        }
        else
        {
            if(abr->droit->cardDonnee==0)
            {
                for(int i = 0;i<abr->cardDonnee;i++)
                {
                    int predict = (int)data[abr->indexDonnee[i]][4];
                    tab[predict] += 1.;
                }
                for(int i = 0;i<3;i++)
                {
                    if (tab[max] <tab[i])
                    {
                        max = i;
                    }
                }
                return  (float)max;
            }
            else
            {
                return(prediction(abr->droit,Indices_test,data));
            }
        }
    }
}
float oob_score(AbrDecision* arbre,int ooblen,float data[MAX_ROWS][MAX_COLS])
{
    float mis_label= 0.;
    for(int i =0 ; i< ooblen;i++)
    {
        float pred = prediction(arbre->racine,data[arbre->oobIndices[i]],data);
        if ( pred !=  (float)data[i][4])
        {
            mis_label += 1.;
        }
    }
    return 1.-mis_label/(float)ooblen;
}

void draw_bootstraprand(int len, Foretalea* foret, float data[MAX_ROWS][MAX_COLS]) 
{
    srand(time(NULL));
    int oob = 4*len/5 ;
    int* bootstrapInd = malloc(sizeof(int) * oob);
    assert(bootstrapInd);
    int* oobInd = malloc(sizeof(int) * (len -oob));
    assert(oobInd);
    int** Combi = malloc(foret->nbEstimateur * sizeof(int*));
    assert(Combi);
    for (int i = 0; i < foret->nbEstimateur; i++)
    {
        Combi[i] = malloc(foret->nbParamParArbre * sizeof(int));
        assert(Combi[i]);
    }
    //génération de l'ensemble des combinaisons de paramètre
    for (int i = 0; i < foret->nbEstimateur; i++) 
    {   //génération des indices des paramètres aléatoires uniques
        for (int j = 0; j < foret->nbParamParArbre; j++) 
        {
            int newIndice = 0;
            int unique = 0;
            while (!unique)
            {
                unique = 1;
                newIndice = rand() % foret->nbParam;
                if (newIndice == 4){
                    unique = 0;
                }
                else{
                //on vérifie l'unicité des indices dans Combi[i]
                for (int k = 0; k < j; k++)
                {
                    if (Combi[i][k] == newIndice)
                    {
                        unique = 0;
                        break;
                    }
                }
                }
            }
            Combi[i][j] = newIndice;
        }
    }
    //utilisation des indices de bootstrap pour créer les arbres de décision de la forêt
    for (int j = 0; j < foret->nbEstimateur; j++)
    {
        for (int i = 0; i < len -oob; i++){
            int newindice = 0;
            bool unique = false;
            while (!unique)
            {
                newindice = rand()%len;
                unique = true;
                for(int k = 0;k<i;k++){
                    if(oobInd[k]==newindice){
                        unique = false;
                        break;
                    }
                } 
            }
            oobInd[i] = newindice;
        }
        int compteur = 0;
        while (compteur!= oob)
        {
            int ind = rand()%len;
            float new = true;
            for (int i = 0; i<(len -oob);i++ ){
                if(ind == oobInd[i]){
                    new = false ;
                    break;
                }
            }
            if(new){
                for(int i = 0;i<compteur;i++){
                    if(ind == bootstrapInd[i]){
                        new = false;
                        break;
                    }
                }
                if(new){
                    bootstrapInd[compteur] = ind;
                    compteur++;
                }
            }
        }
        foret->forest[j] = creer_arbre(Combi[j], bootstrapInd,oobInd,foret->nbParamParArbre,oob, len-oob,foret->profMax,data);
    }
    float oobscore = 0.;
    for (int i = 0; i<foret->nbEstimateur;i++)
    {
        oobscore += oob_score(foret->forest[i],len-oob,data);
    }
    foret->fiabilite = oobscore/(float)foret->nbEstimateur;
}

float prediction_foret(Foretalea* foret,float* test,float data[MAX_ROWS][MAX_COLS])
{
    //même principe que prediction mais on interroge chaque arbre
    float tab[3] = {0.,0.,0.};
    int max = 0;
    for(int i = 0;i<foret->nbEstimateur;i++)
    {
        int predict = (int)prediction(foret->forest[i]->racine,test,data);
        tab[predict] += 1.;
    }
    for(int i = 0;i<3;i++)
    {
        if (tab[max] <tab[i])
        {
            max = i;
        }
    }
    return (float)max;
}
//libération de la mémoire allouée
void freenoeud(Noeud* node)
{
    if(!node->feuille)
    {
        freenoeud(node->droit);
        freenoeud(node->millieu);
        freenoeud(node->gauche);
    }
    free(node);
}
void freearbre(AbrDecision* arbre)
{
    freenoeud(arbre->racine);
    free(arbre);
}

void freeforet(Foretalea* foret)
{
    for (int i = 0; i<foret->nbEstimateur;i++)
    {
        freearbre(foret->forest[i]);
    }
    free(foret->forest);
    free(foret);
}

int main(){
    //récupération des données
    FILE*file;
    file = fopen("C:\\Users\\Desktop\\Prepa\\TIPE_5_2\\database\\SQLite.csv", "r");
    if (file == NULL) 
    {
        perror("Impossible d'ouvrir le fichier");
        return 1;
    }
    
    int rows = 380;
    int cols = 65;
    char buffer[500] ;
    char *record,*line;
    
    //détermine le nombre de lignes et de colonnes

    //alloue la mémoire pour la matrice
    float matrix[rows][cols];
    int row = 0;
    int col = 0;
    
    while((line=fgets(buffer,sizeof(buffer),file))!=NULL)
    {
        record = strtok(line,",");
        while(record != NULL)
        {
        
            if(col<cols)
            {           
                matrix[row][col] = atof(record);
                col++;
                record = strtok(NULL,",\n");
            }
        }
        if(row<rows)
        {
            col = 0;
            row++;        
        }
    }
    fclose(file);
    time_t begin = time( NULL );
    //création de la forêt
    Foretalea* foret = malloc(sizeof(Foretalea));
    assert(foret);
    foret->nbEstimateur = 100;
    foret->forest = malloc(sizeof(AbrDecision*)*foret->nbEstimateur);
    assert(foret->forest);
    for (int i = 0; i<foret->nbEstimateur;i++)
    {
        foret->forest[i] = malloc(sizeof(AbrDecision*));
        assert(foret->forest[i]);
    }
    assert(foret->forest != NULL);
    foret->profMax = 15;
    foret->nbFeuilleMin = 10;
    foret->nbSeparationMin = 10;
    foret ->nbParamParArbre = 10;
    foret ->nbParam = 60;
    //entrainement de la forét
    draw_bootstraprand(250,foret,matrix);
    //test de sa fiabilité
    float mis_label= 0.;
    for(int i =0 ; i< 50;i++)
    {
        float pred = prediction_foret(foret,matrix[i+250],matrix);
        if ( pred !=  (float)matrix[i][4])
        {
            mis_label += 1.;
        }
    }
    printf("fiabilité foret: %f %% \n",100.*(1.-mis_label/50.));
    freeforet(foret);
    time_t end = time( NULL);
    unsigned long secondes = (unsigned long) difftime( end, begin );
    printf( "Finie en %ld sec\n", secondes );
    return 0;
}
