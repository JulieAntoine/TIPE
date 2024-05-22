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
#define MAX_COLS 10  // Nombre maximum de colonnes dans le fichier CSV
#define NB_donnée 380 // Nombre de donnée
#define NUM_FEATURES 6 //Nombre d'exemple par donnée
#define NB_Param 10// Nombre de paramètre par arbre
float table[NB_donnée][NUM_FEATURES];
//on représente une victoire par un 1 et une défaite par un -1 dans la colonne [4]
// structures
struct Noeud { //rajouter un indice a Node pour définir une fonction de poid
    int* index_donnee;  //tableau contenant les indices des données possédés
    int seuil;          //Seuil permettra de parcourir l'abre créé
    int* indParams;     //tableau contenant les indices des paramétres possédés
    int card;           //taille du tableau des paramètres
    int paramSplit;     //Indices du paramétre sur lequel on split
    int hauteur;        //hautuer permet de connaitre où se trouve dans l'abre
    struct Noeud* gauche;
    struct Noeud* droit;
};
typedef struct Noeud Noeud;

struct AbrDecision {// rajouter valuation et NOM en int
    int profondeur_max;
    int min_samples_split;
    int min_samples_leaf;
    Noeud* tree;
};
typedef struct AbrDecision AbrDecision;

struct Foretalea{
    int nbEstimateur;
    int max_depth;
    int min_samples_split;
    int min_samples_leaf;
    int nbParamParArbre;
    AbrDecision** forest;
};
typedef struct Foretalea Foretalea;


Noeud* creerNoeud(int* index, float seuil, int* param, int card,int hauteur) {
    Noeud* a = malloc(sizeof(Noeud));
    a->gauche = malloc(sizeof(Noeud));
    assert(a->gauche!= NULL);
    a->droit = malloc(sizeof(Noeud));
    assert(a->droit!= NULL);
    a->index_donnee = index;
    a->seuil = seuil;
    a->indParams = param;
    a->card = card;
    a->hauteur = hauteur;
    a->paramSplit = -1;
    return a;
}



int** parmi(int n, int k,int *taille_resultat ) { //renvoi un tableau contenant toutes combinaisons de k élements parmi n
    int compteur = 0;
    int taille = k;
    int** res = malloc(sizeof(int*) * taille);
    int* tab_indice = malloc(sizeof(int) * k); // initialisation des positions initiales
    for (int i = 0; i < k; i++) {
        tab_indice[i] = i + 1;
    }

    bool possible = true;
    while (possible) {
        if (compteur == taille) {
            // Si on dépasse la taille maximale, on double la taille manuellement
            int nouvelle_taille = taille * 2;
            int** nouveau_res = malloc(sizeof(int*) * nouvelle_taille);
            for (int i = 0; i < taille; i++) {
                nouveau_res[i] = res[i];
            }
            free(res); // Libérer l'ancien tableau
            res = nouveau_res;
            taille = nouvelle_taille;
        }
        
        res[compteur] = malloc(sizeof(int) * k);
        // on stock la combinaison 
        for (int i = 0; i < k; i++) {
            res[compteur][i] = tab_indice[i];
        }
        compteur++; 

        // Trouver l'indice à incrémenter
        int indice = k - 1;
        while (tab_indice[indice] >= n - k + indice + 1 && indice >= 0) {
            indice--;
        }

        // Si aucun indice valide n'est trouvé, nous avons fini
        if (indice < 0) {
            possible = false;
        } else {
            // Incrémenter l'indice trouvé
            tab_indice[indice] += 1;
            
            // Mettre à jour les indices suivants
            for (int j = indice + 1; j < k; j++) {
                tab_indice[j] = tab_indice[j - 1] + 1;
            }
        }
    }
    free(tab_indice); // Libérer la mémoire allouée pour tab_indice
    *taille_resultat = compteur; // Mettre à jour la taille réelle du tableau de résultats
    
    return res;
}

float entropy(int indices[], int card) {//indices contient les indices de tout les éléments considérés et card en est le nombre
   float vict = 0.0;
   for (int i = 0; i<card ;i++){
    if (table[4][indices[i]]== 1){
        vict+= 1.0/card;
    }
   }
   if (vict == 0.0 || vict == 1){
    return 0.0;
   }
   else{
    return -vict*log10f(vict)-(1.0-vict)*log10f(1.0-vict);
   }
}


float informationGain(int indices[],int card,int param) {//indices contient les indices dans table de l'ensemble qu'on considére, card son cardinal, param l'indice du paramètre par rapport auquel on considére le split
    float HS = entropy(indices,card); //entropy du parent
    int separatrice = 0;
    for (int i =0; i<card; i++){ //On calcule la moyenne du paramétre choisi pour le split 
        separatrice += table[param][i]/card;
    }
    int ind_moins[card/2];
    int ind_plus[card/2];
    int moins = 0;
    int plus = 0;
    for (int i = 0 ; i<card;i++){ //On sépare l'ensemble considéré entre les valeurs supérieur et inférieur à la moyenne
        if (table[param][indices[i]]<separatrice){
            ind_moins[moins] = indices[i];
            moins++;
        }
        else{
            ind_plus[plus] = indices[i];
            plus++;
        }
    }
    return HS - (1/2)*entropy(ind_moins,card/2) -(1/2)*entropy(ind_plus,card/2);
}

Noeud* trouverPointdeSplit(int params[],int indices[],int card,int hauteur){ //params contient l'ensemble des indices considérés pour créer un arbre donné, indices l'ensemble des indices que l'on considére
    float Gain_max = 0.0;                                                   
    int indgainmax = 0;
    int ind_moins[card/2];
    int ind_plus[card/2];
    int moins = 0;
    int plus = 0;
    for (int i = 0; i<NB_Param; i++){
        float temp = informationGain(indices,card,params[i]);
        if (Gain_max<temp){
            Gain_max = temp;
            indgainmax = params[i];
        }
    }
    int separatrice = 0;
    for (int i =0; i<card; i++){ //On calcule la moyenne du paramétre choisi pour le split 
        separatrice += table[indgainmax][i]/card;
    }
    for (int i = 0 ; i<card;i++){ //On sépare l'ensemble considéré entre les valeurs supérieur et inférieur à la moyenne
        if (table[indgainmax][indices[i]]<separatrice){
            ind_moins[moins] = indices[i];
            moins++;
        }
        else{
            ind_plus[plus] = indices[i];
            plus++;
        }
    }
    Noeud* parent = creerNoeud(indices,separatrice,params,card,hauteur);
    parent->paramSplit = indgainmax;
    int diff = 0;
    for (int i = 0; i<NB_Param; i++){
        if (params[i]!= indgainmax){
            diff = i;
        }
        if (params[i] == indgainmax){
            params[i] = params[diff];
        }
    }
    Noeud* fg = creerNoeud(ind_moins,0.0,params,card/2,hauteur+1);
    Noeud* fd = creerNoeud(ind_plus,0.0,params,card/2,hauteur+1);
    parent ->droit = fd;
    parent ->gauche = fg;
    return parent;
}
Noeud* creaParcourNoeud(Noeud* racine, int prof_max)
{
    if (racine->hauteur == prof_max) //si la profondeur max est atteinte on arréte de split
    {
        return racine;
    }
    else    //Sinon on a affecte a fils droit et fils gauche la valeur de leur split (split renvoie le Noeud dont les fils sont les deux ensembles obtenus)
    {
        Noeud* filsDroit = trouverPointdeSplit(racine->droit->index_donnee,racine->droit->indParams,racine->droit->card,racine->hauteur);
        Noeud* filsGauche = trouverPointdeSplit(racine->gauche->index_donnee,racine->gauche->indParams,racine->gauche->card,racine->hauteur);
        racine->gauche->gauche = filsGauche->gauche;
        racine->gauche->droit = filsGauche->droit;
        racine->droit->gauche = filsDroit->gauche;
        racine->droit->droit = filsDroit->droit;
        racine->gauche = creaParcourNoeud(filsGauche, prof_max);
        racine->droit = creaParcourNoeud(filsDroit,prof_max);
        free(filsDroit);
        free(filsGauche);
        return racine;
    }
}

AbrDecision* creer_arbre(int* ensparam,int* indices,int card,int profondeur_max){
    Noeud* arbre = creerNoeud(indices,0,ensparam,card,0);
    arbre = creaParcourNoeud(arbre,profondeur_max);
    AbrDecision* abr = malloc(sizeof(AbrDecision));
    abr->tree = malloc(sizeof(Noeud*));
    assert(abr->tree!= NULL);
    abr->min_samples_leaf= 5;
    abr->min_samples_split = 5;
    abr->profondeur_max = profondeur_max;
    abr-> tree = arbre ;
    return abr; 
}


float prediction(Noeud* abr, float* X_test[NUM_FEATURES],float data[MAX_COLS][MAX_ROWS]) {
    if (abr->gauche == NULL && abr->droit == NULL){
        return data[abr->index_donnee[0]][4];
    }
    else {
        if(*X_test[abr->paramSplit]<= abr->seuil){
            return prediction(abr->gauche,X_test,data);
        }
        else{
            return prediction(abr->droit,X_test,data);
        }
    }
}

/*
void draw_bootstraptot(int len,Foretalea* foret) { //len représente la quantité de paramètre totale
    int oob = len/10;
    int* bootstrapInd = malloc(sizeof(int)*(len-oob));
    int* oobInd = malloc(sizeof(int)*oob);
    for (int i = 0;i<len-oob  ;i++){
        bootstrapInd[i] = i;
    }
    for (int i = len - oob;i<len+1;i++){
        oobInd[i] = i;
    } 
    int** Combi = Parmi(foret->nbParamParArbre,foret->nbParamParArbre+4);
    for (int j = 0; j<foret->nbEstimateur; j++){
        foret->forest[j] = creer_arbre(Combi[j],bootstrapInd,len-oob,foret->max_depth);
    }
    free(Combi);
    free(bootstrapInd);
    free(oobInd);
} */

float oob_score(Noeud* arbre,int* OobInd,int taille_Oob,float data[MAX_COLS][MAX_ROWS]) {
    int mis_label =0;
    for(int i =0 ; i< taille_Oob;i++){
        int pred = prediction(arbre,data[OobInd[i]],data);
        if ( pred !=  data[4][i]){
            mis_label++;
        }
    }
    return mis_label/taille_Oob;

}

void draw_bootstraprand(int len, Foretalea* foret, float data[MAX_COLS][MAX_ROWS]) {
    srand(time(NULL));
    int oob = 2*len/3 ;
    int* bootstrapInd = malloc(sizeof(int) * oob);
    int* oobInd = malloc(sizeof(int) * (len -oob));
    int** Combi = malloc(foret->nbEstimateur * sizeof(int*));
    for (int i = 0; i < foret->nbEstimateur; i++)
    {
        Combi[i] = malloc(foret->nbParamParArbre * sizeof(int));
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
                newIndice = rand() % len;
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
            Combi[i][j] = newIndice;
        }
    }
    for (int i = 0; i < oob; i++) 
    {
        bootstrapInd[i] = i;
    }
    // Utiliser les indices de bootstrap pour créer les arbres de décision de la forêt
    for (int j = 0; j < foret->nbEstimateur; j++)
    {
        foret->forest[j] = creer_arbre(Combi[j], bootstrapInd, oob, foret->max_depth);
    }
    // Générer des indices OOB (out-of-bag)
    int index = 0;
    for (int i = 0; i < len; i++) {
        int isOOB = 1; // Marquer l'indice comme OOB par défaut
        for (int j = 0; j < len - oob; j++) {
            if (i == bootstrapInd[j]) {
                isOOB = 0; // L'indice n'est pas OOB s'il est dans les indices de bootstrap
                break;
            }
        }
        if (isOOB) {
            oobInd[index++] = i; // Ajouter l'indice OOB
        }
    }
    int oobscore = 0;
    for (int i = 0; i<foret->nbEstimateur;i++)
    {
        oobscore += oob_score(foret->forest[i],oobInd,len-oob,data);
    }
    printf("%d",oobscore/foret->nbEstimateur);
    // Libérer la mémoire
    for (int i = 0; i < foret->nbEstimateur; i++) {
        free(Combi[i]);
    }
    free(Combi);
    free(bootstrapInd);
    free(oobInd);
}







int main() {
    // Define variables and load dataset
    FILE *file = fopen("Résultats.csv", "r");
    if (file == NULL) {
        perror("Impossible d'ouvrir le fichier");
        return 1;
    }

    char line[1024];
    int rows = 0;
    int cols = 0;
    float data[MAX_ROWS][MAX_COLS]; // Matrice pour stocker les données

    while(fgets(line, 1024, file) && rows < MAX_ROWS) {
        char *token = strtok(line, ",");
        cols = 0;
        while (token != NULL && cols < MAX_COLS) {
            data[rows][cols] = atof(token); // Conversion de la chaîne en float
            token = strtok(NULL, ",");
            cols++;
        }
        rows++;
    }

    fclose(file);
    // Initialize RandomForest parameters
    Foretalea* foret = malloc(sizeof(Foretalea));
    foret->nbEstimateur = 50;
    foret->forest = malloc(sizeof(AbrDecision*)*foret->nbEstimateur);
    for (int i = 0; i<foret->nbEstimateur;i++){
        foret->forest = malloc(sizeof(AbrDecision*));
    }
    assert(foret->forest != NULL);
    foret->max_depth = 15;
    foret->min_samples_leaf = 10;
    foret->min_samples_split = 10;
    foret ->nbParamParArbre = 6 ;
    // Train the RandomForest
    draw_bootstraprand(100,foret,data);
    // Predict using the trained RandomForest

    return 0;
}
