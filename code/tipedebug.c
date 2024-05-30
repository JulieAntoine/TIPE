#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

 //Penser a timer pour déterminer le temps pris
 // Random ou toutes combinaisons possibles?
 //fonction créer arbre et boostrap
#define MAX_ROWS 380 // Nombre maximum de lignes dans le fichier CSV
#define MAX_COLS 65  // Nombre maximum de colonnes dans le fichier CSV
#define MAX_LINE_LENGTH 65
//on représente une victoire par un 1 et une défaite par un 0 dans la colonne [4]
// structures
struct Noeud { //rajouter un indice a Node pour définir une fonction de poid
    int* index_donnee;  //tableau contenant les indices des données possédés
    int card_donnee;    //taille de index_donnee
    float seuil;          //Seuil permettra de parcourir l'abre créé
    int* indParams;     //tableau contenant les indices des paramétres possédés
    int cardParam;      //taille de IndParams
    int paramSplit;     //Indices du paramétre dans IndParam sur lequel on split pour aller à fg ou fd
    int hauteur;       //hauteur permet de connaitre où se on  trouve dans l'abre
    bool feuille;      //permet de savoir si on se trouve sur une feuille lors d'un parcour
    struct Noeud* gauche;
    struct Noeud* droit;
};
typedef struct Noeud Noeud;

struct AbrDecision {// rajouter valuation et NOM en int
    int profondeur_max;         //Permet de connaitre la profondeur maximum de parcous
    int min_samples_split;      //Permet d'éviter d'avoir des arbres de taille trop faible
    int min_samples_leaf;       //Pour éviter les abres de trop faible dispersion
    int* oob;                   //tableau des indices out of bag pour cet arbre
    int card;                   //cardinal de oob
    Noeud* tree;
};
typedef struct AbrDecision AbrDecision;

struct Foretalea{
    int nbEstimateur;       //Nombre d'arbre voulu
    int max_depth;          //profondeur maximale par arbre
    int min_samples_split;  //
    int min_samples_leaf;
    int nbParamParArbre;    //combien de Paramétre par arbre
    int nbParam;            //Nombre de Paramétre totale
    float fiabilite;        //fiabilité de la forét
    AbrDecision** forest;
};
typedef struct Foretalea Foretalea;

//fonctions pour afficher (utile pour vérifier et débugguer)
void affiche(Noeud* arbre){
    // printf("indyyy %d \n",arbre->index_donnee[0]);
    if(arbre->card_donnee== 0){
        printf("! Warning Manque de donnée !\n");
    }
    else{
    printf("Hauteur %d ",arbre->hauteur);
    printf("Separatrice : %f ",arbre->seuil);
    printf("Card donnée : %d ",arbre->card_donnee);
    if( arbre->feuille){
        printf("feuille ");
    }
    else{
    printf("Paramsplit : %d ",arbre->indParams[arbre->paramSplit]);}
    printf("Index: [");
    for (int i = 0; i < arbre->card_donnee ; i++)
    {
        printf("%d,",arbre->index_donnee[i]);
    }
    printf("] Param [");
    for (int i = 0; i<arbre->cardParam;i++){
        printf("%d,",arbre->indParams[i]);
    }
    printf("]\n");
    if (arbre->feuille){
        // printf("fin \n");
    }
    else{
    // printf("pointeur2 %p \n",arbre->gauche);
    printf("<-\n");   
    affiche(arbre->gauche);
    // printf("pointeur3 %p \n",arbre->gauche);
    printf("->\n");
    affiche(arbre->droit);
    }
    }
}

void affiche_arbre(AbrDecision* arbre){
    printf(" bootstrap : [");
    for(int i = 0 ;i<arbre->card;i++){
        printf(" %d,",arbre->oob[i]);
    }
    printf("] \n");
    affiche(arbre->tree);
}

Noeud* creerNoeud(int* index, int cardIndex, float separatrice, int* IndParams, int cardParam,int hauteur) {
    Noeud* a = malloc(sizeof(Noeud));
    a->gauche = malloc(sizeof(Noeud));
    assert(a->gauche!= NULL);
    a->droit = malloc(sizeof(Noeud));
    assert(a->droit!= NULL);
    a->index_donnee = index;
    a->card_donnee = cardIndex;
    a->seuil = separatrice;
    a->indParams = IndParams;
    a->cardParam = cardParam;
    a->hauteur = hauteur;
    a->paramSplit = -1;
    a->feuille = true;
    return a;
}

float entropy(int* indices, int card,float data[MAX_ROWS][MAX_COLS]) //indices contient les indices de tout les éléments considérés et card en est le nombre
{   
//    printf("\n ok-entropy %d",card);
   float vict = 0.0;
   for (int i = 0; i<card ;i++)
   {
    // printf("\n indice %d",i);
    // printf("\n %d",indices[i]);
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
float* informationGain(int* indices,int card,int param,float data[MAX_ROWS][MAX_COLS]) {//indices contient les indices dans table de l'ensemble qu'on considére, card son cardinal, param l'indice du paramètre par rapport auquel on considére le split
    float HS = entropy(indices,card,data); //entropy du parent
    float separatrice = 0;
    for (int i =0; i<card; i++) //On calcule la moyenne du paramétre choisi pour le split 
    {   
        // printf("\n i %f",data[i][param]/card);    
        separatrice += (float)data[i][param];
    }
    separatrice = separatrice/(float)card;
    // printf("\nseparatrice: %d",separatrice);
    int *supsou = malloc(sizeof(int)*card); //permet de connaitre quel élément de indices ira a gauche et lequel à droite
    assert(supsou);
    int moins = 0;
    int plus = 0;
    for (int i = 0 ; i<card;i++){ //On sépare l'ensemble considéré entre les valeurs supérieur et inférieur à la moyenne
        if ((float)data[indices[i]][param]<separatrice){
            supsou[i] = -1;     //-1 pour gauche
            //printf("\n %d",indices[i]);
            moins++;
        }
        else{
            //printf("\n %d",indices[i]);
            supsou[i] == 1; //1 pour droite
            plus++;
        }
    }
    int* ind_moins = malloc(sizeof(int)*moins);
    int* ind_plus = malloc(sizeof(int)*plus);
    moins =0;
    plus = 0;
    for (int i = 0;i<card;i++){
        if(supsou[i]==-1){
            ind_moins[moins] = indices[i];
            moins++;
        }
        else{
            ind_plus[plus] = indices[i];
            plus++;
        }
    }
    // printf("\n %d",plus);
    free(supsou);
    // printf("HS %f\n",HS);
    // printf("entro+ %f\n",entropy(ind_plus,plus,data));
    // printf("entro- %f\n",entropy(ind_moins,moins,data));
    float res = HS - ((float)moins/(float)card)*entropy(ind_moins,moins,data) -((float)plus/(float)card)*entropy(ind_plus,plus,data);
    float* tabres = malloc(sizeof(float)*(moins+plus+4)); //on stocke toutes les informations intéressante dans un tableau pour ne pas réeffectuer le calcul dans la prochaine fonction
    assert(tabres);
    tabres[0] = res;
    tabres[1] = (float)moins;
    tabres[2] = (float)plus;
    tabres[3] = separatrice;
    for (int i = 0;i<moins;i++){
        tabres[i+4] = (float)ind_moins[i];
    }
    for (int i = 0;i<plus;i++){
        tabres[i+4+moins] = (float)ind_plus[i];
    }
    return tabres; //calcul du gain d'information
}

Noeud* trouverPointdeSplit(int params[],int indices[], int card_donnee,int cardParam,int hauteur,float data[MAX_ROWS][MAX_COLS]){ //params contient l'ensemble des indices considérés pour créer un arbre donné, indices l'ensemble des indices que l'on considére
    // printf(" ok-trouverptsplit\n");
    // printf("card_donne %d\n",card_donnee);
    //printf("\n ind1 %d",indices[0]);
    float Gain_max = 0.0;                                                   
    int indgainmax = 0;
    float* ind_gain = malloc(sizeof(float)*(card_donnee+3));
    int* Paramsenfant = malloc(sizeof(int)*(cardParam-1) ); //On va donner à chaque enfant les mêmes paramètres excepté celui pour split
    // printf("cardparam %d \n",cardParam);
    assert(Paramsenfant);
    assert(ind_gain);
    for (int i = 0; i<cardParam; i++)
    { 
        float* temp = informationGain(indices,card_donnee,params[i],data);
        // printf("%f \n",temp[0]);
        if (Gain_max<temp[0]){
            Gain_max = temp[0];
            ind_gain = temp;
            indgainmax = i;
        }
    }
    int moins = (int)ind_gain[1];
    int plus = (int)ind_gain[2];
    int* ind_moins = malloc(sizeof(int)*moins);
    int* ind_plus = malloc(sizeof(int)*plus);
    for (int i = 0; i<moins;i++){
       
        ind_moins[i] = (int)ind_gain[i+4];
        
    }
    for (int i = 0; i<plus;i++){
        ind_plus[i] = (int)ind_gain[i+4+moins];
    }
    float separatrice = ind_gain[3];
    //printf("\n ind2 %d",indices[0]);
    Noeud* parent = creerNoeud(indices,card_donnee,separatrice,params,cardParam,hauteur);
    //printf("\n ind %d",indices[0]);
    parent->paramSplit = indgainmax;
    int compteur = 0;
    for (int i = 0; i<cardParam;i++)
    {
        if(i != indgainmax){
            Paramsenfant[ compteur] = params[i];
            compteur++;
        }
    }
    // printf("moins %d \n",moins);
    // printf("plus %d \n",plus);
    Noeud* fg = creerNoeud(ind_moins,moins,0.0,Paramsenfant,cardParam-1,hauteur+1);
    Noeud* fd = creerNoeud(ind_plus,plus,0.0,Paramsenfant,cardParam-1,hauteur+1);
    parent ->droit = fd;
    parent ->gauche = fg;
    parent->feuille = false;
    return parent;
}

Noeud* creaParcourNoeud(Noeud* racine, int prof_max,float data[MAX_ROWS][MAX_COLS])
{
    // printf("\n ok-creerparcour");
    if (racine->hauteur == prof_max || racine->card_donnee == 1||racine->card_donnee==0||racine->cardParam==0) //si la profondeur max est atteinte ou si il n'y a plus assez de donnée on arréte de split
    {
        return racine;
    }
    else    //Sinon on a affecte a fils droit et fils gauche la valeur de leur split (split renvoie le Noeud dont les fils sont les deux ensembles obtenus)
    {   
        // printf("card racine %d \n ",racine->card_donnee);
        // printf(" ind %d \n ",racine->index_donnee[0]);
        Noeud* fils = trouverPointdeSplit(racine->indParams,racine->index_donnee, racine->card_donnee, racine->cardParam, racine->hauteur,data);
        racine->seuil = fils->seuil;
        racine->gauche = fils->gauche;
        racine->droit = fils->droit;
        racine->gauche = creaParcourNoeud(racine->gauche, prof_max,data); //on applique ensuite récursivement le principe de création d'un arbre
        racine->droit = creaParcourNoeud(racine->droit,prof_max,data);
        racine->feuille = false;
        
        racine->paramSplit = fils->paramSplit;
        // printf(" ind %d \n ",racine->index_donnee[0]);
        free(fils);
        return racine;
    }
}

AbrDecision* creer_arbre(int* ensparam,int* indices,int*boots,int cardParam,int cardIndex,int cardboots,int profondeur_max,float data[MAX_ROWS][MAX_COLS]){
    // printf("\n ok-crearbre");
    Noeud* arbre = creerNoeud(indices,cardIndex,0,ensparam,cardParam,0);
    // printf("%d",arbre->card_donnee);
    // printf("\n ind1%d",indices[0]);
    arbre = creaParcourNoeud(arbre,profondeur_max,data);
    // printf("\n ind2 %d",indices[0]);
    AbrDecision* abr = malloc(sizeof(AbrDecision));
    abr->tree = malloc(sizeof(Noeud*));
    assert(abr->tree!= NULL);
    abr->min_samples_leaf= 5;
    abr->min_samples_split = 5;
    abr->profondeur_max = profondeur_max;
    abr-> tree = arbre ;
    abr->oob = boots;
    abr->card = cardboots;
    return abr; 
}

float prediction(Noeud* abr,float Indices_test[],float data[MAX_ROWS][MAX_COLS]) {
    // printf("\n ok-prediction");
    if (abr->feuille){
        return data[abr->index_donnee[0]][4];
    }
    else{
        if(Indices_test[abr->indParams[abr->paramSplit]] < abr->seuil){
            if(abr->gauche->card_donnee==0){
                return data[abr->index_donnee[0]][4];
            }
            else{
            return(prediction(abr->gauche,Indices_test,data));
            }
        }
        else{
            if(abr->droit->card_donnee==0){
                return data[abr->index_donnee[0]][4];
            }
            else{
            return(prediction(abr->droit,Indices_test,data));
            }
        }
    }
}
float oob_score(AbrDecision* arbre,int ooblen,float data[MAX_ROWS][MAX_COLS]){
    float mis_label= 0.;
    for(int i =0 ; i< ooblen;i++){
        float pred = prediction(arbre->tree,data[arbre->oob[i]],data);
        if ( pred !=  (float)data[i][4]){
            mis_label += 1.;
        }
    }
    /*printf("mis label: %f \n",mis_label);
    printf("oob len  : %f \n",(float)ooblen);
    printf("fiabilité : %f \n",1.-mis_label/(float)ooblen);*/
    return 1.-mis_label/(float)ooblen;
}

void draw_bootstraprand(int len, Foretalea* foret, float data[MAX_ROWS][MAX_COLS]) {
    // printf(" ok-bootstrap\n");
    srand(time(NULL));
    int oob = 2*len/3 ;
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
    //Génération de l'ensembles de combinaison de paramètre
    for (int i = 0; i < foret->nbEstimateur; i++) 
    {   // Génération des indices des paramètres aléatoires uniques
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
                // Vérifier l'unicité des indices dans Combi[i]
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
    // Utiliser les indices de bootstrap pour créer les arbres de décision de la forêt
    for (int j = 0; j < foret->nbEstimateur; j++)
    {
        // printf("\n boot %d",bootstrapInd[0]);
        // printf("\n oob %d",oob);
        // Générer des indices OOB (out-of-bag)
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
        
        foret->forest[j] = creer_arbre(Combi[j], bootstrapInd,oobInd,foret->nbParamParArbre,oob, len-oob,foret->max_depth,data);
    }
    float oobscore = 0.;
    for (int i = 0; i<foret->nbEstimateur;i++)
    {
        oobscore += oob_score(foret->forest[i],len-oob,data);
    }
    foret->fiabilite = oobscore/(float)foret->nbEstimateur;
}

float prediction_foret(Foretalea* foret,float* test,float data[MAX_ROWS][MAX_COLS]){
    float tab[3] = {0.,0.,0.};
    int max = 0;
    for(int i = 0;i<foret->nbEstimateur;i++){
        int predict = (int)prediction(foret->forest[i]->tree,test,data);
        tab[predict] += 1.;
    }
    for(int i = 0;i<3;i++){
        if (tab[max] <tab[i]){
            max = i;
        }
    }
    return (float)max;
}

int main(){

    FILE*file;
    file = fopen("C:\\Users\\valen\\Desktop\\ProgrammesC\\Resultats.csv", "r");
    if (file == NULL) {
        perror("Impossible d'ouvrir le fichier");
        return 1;
    }
    
    int rows = 380;
    int cols = 65;
    char buffer[500] ;
    char *record,*line;
    
    // First pass to determine the number of rows and columns

    // Allocate memory for the matrix
    float matrix[rows][cols];
    int row = 0;
    int col = 0;
    
    while((line=fgets(buffer,sizeof(buffer),file))!=NULL)
   {
     record = strtok(line,",");
     while(record != NULL)
     {
        
        if(col<cols){
            
            matrix[row][col] = atof(record);
            col++;
            record = strtok(NULL,",\n");
        }
     }
     if(row<rows){
        col = 0;
        row++;
        
     }
   }
    fclose(file);
    Foretalea* foret = malloc(sizeof(Foretalea));
    assert(foret);
    foret->nbEstimateur = 40;
    foret->forest = malloc(sizeof(AbrDecision*)*foret->nbEstimateur);
    assert(foret->forest);
    for (int i = 0; i<foret->nbEstimateur;i++){
        foret->forest[i] = malloc(sizeof(AbrDecision*));
        assert(foret->forest[i]);
    }
    assert(foret->forest != NULL);
    foret->max_depth = 15;
    foret->min_samples_leaf = 10;
    foret->min_samples_split = 10;
    foret ->nbParamParArbre = 10 ;
    foret ->nbParam = 60;
    // Train the RandomForest
    draw_bootstraprand(350,foret,matrix);
    float mis_label= 0.;
    for(int i =0 ; i< 30;i++){
        float pred = prediction_foret(foret,matrix[i+350],matrix);
        if ( pred !=  (float)matrix[i][4]){
            mis_label += 1.;
        }
    }
    printf("\n");
    printf("fiabilité moyenne : %f %% \n",100.*foret->fiabilite);
    printf("fiabilité foret : %f %% \n",100*(mis_label/30.));
    // printf("inddd %d ",arbre2->gauche->index_donnee[0]);
    // affiche_arbre(arbre2);
    // printf("\n IG %f ",informationGain(&tab,6,4,matrix));
    // Print the matrix to verify
    /*for (int i = 0; i < rows; i++) {
        printf("[");
        for (int j = 0; j < cols; j++) {
            printf("%lf ,", matrix[i][j]);
        }
        printf("]\n");
    }
    printf("rows %d",rows);*/
    // Free allocated memory
    return 0;
}