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
//on représente une victoire par un 1 et une défaite par un 0 dans la colonne [4]
// structures
struct Noeud { //rajouter un indice a Node pour définir une fonction de poid
    int* index_donnee;  //tableau contenant les indices des données possédés
    int card_donnee;    //taille de index_donnee
    int seuil;          //Seuil permettra de parcourir l'abre créé
    int* indParams;     //tableau contenant les indices des paramétres possédés
    int cardParam;      //taille de IndParams
    int paramSplit;     //Indices du paramétre sur lequel on split pour aller à fg ou fd
    int hauteur;        //hauteur permet de connaitre où se on  trouve dans l'abre
    struct Noeud* gauche;
    struct Noeud* droit;
};
typedef struct Noeud Noeud;

struct AbrDecision {// rajouter valuation et NOM en int
    int profondeur_max;         //Permet de connaitre la profondeur maximum de parcous
    int min_samples_split;      //Permet d'éviter d'avoir des arbres de taille trop faible
    int min_samples_leaf;
    Noeud* tree;
};
typedef struct AbrDecision AbrDecision;

struct Foretalea{
    int nbEstimateur;       //Nombre d'arbre voulu
    int max_depth;          
    int min_samples_split;  
    int min_samples_leaf;
    int nbParamParArbre;
    AbrDecision** forest;
};
typedef struct Foretalea Foretalea;


Noeud* creerNoeud(int* index, int cardIndex, float seuil, int* IndParams, int cardParam,int hauteur) {
    printf("\n ok-creerNoeud \n");
    Noeud* a = malloc(sizeof(Noeud));
    a->gauche = malloc(sizeof(Noeud));
    assert(a->gauche!= NULL);
    a->droit = malloc(sizeof(Noeud));
    assert(a->droit!= NULL);
    a->index_donnee = index;
    a->card_donnee = cardIndex;
    a->seuil = seuil;
    a->indParams = IndParams;
    a->cardParam = cardParam;
    a->hauteur = hauteur;
    a->paramSplit = -1;
    return a;
}



/*int** parmi(int n, int k,int *taille_resultat ) { //renvoi un tableau contenant toutes combinaisons de k élements parmi n
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
} */

float entropy(int* indices, int card,float data[MAX_ROWS][MAX_COLS]) //indices contient les indices de tout les éléments considérés et card en est le nombre
{   
   printf("\n ok-entropy");
   float vict = 0.0;
   printf("\n a%d",card);
   for (int i = 0; i<card ;i++)
   {
    printf("\n indice %d",i);
    printf("\n %d",indices[i]);
    if (data[4][indices[i]]== 1.0)
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


float informationGain(int* indices,int card,int param,float data[MAX_ROWS][MAX_COLS]) {//indices contient les indices dans table de l'ensemble qu'on considére, card son cardinal, param l'indice du paramètre par rapport auquel on considére le split
    printf("\n ok-IG");
    float HS = entropy(indices,card,data); //entropy du parent
    int separatrice = 0;
    printf("\n card %d",card);
    printf("\n param %d",param);
    for (int i =0; i<card; i++) //On calcule la moyenne du paramétre choisi pour le split 
    {   
        printf("\n %d",data[param][i]);    
        separatrice += data[param][i]/card;
    }
    int ind_moins[card/2];
    int ind_plus[card/2];
    int moins = 0;
    int plus = 0;
    printf("\n separatrice %d",separatrice);
    for (int i = 0 ; i<card;i++){ //On sépare l'ensemble considéré entre les valeurs supérieur et inférieur à la moyenne
        if (data[param][indices[i]]<separatrice){
            ind_moins[moins] = indices[i];
            //printf("\n %d",indices[i]);
            moins++;
        }
        else{
            //printf("\n %d",indices[i]);
            ind_plus[plus] = indices[i];
            plus++;
        }
    }
    return HS - (1/2)*entropy(ind_moins,card/2,data) -(1/2)*entropy(ind_plus,card/2,data); //calcul du gain d'information
}

Noeud* trouverPointdeSplit(int params[],int indices[], int card_donnee,int cardParam,int hauteur,float data[MAX_ROWS][MAX_COLS]){ //params contient l'ensemble des indices considérés pour créer un arbre donné, indices l'ensemble des indices que l'on considére
    printf("\n ok-trouverptsplit");
    if (cardParam == 0) //si il n'y a plus de paramaètre on renvoie la feuille formée des données restantes
    {
        return creerNoeud(indices,card_donnee,0.0,params,cardParam,hauteur);
    }
    float Gain_max = 0.0;                                                   
    int indgainmax = 0;
    int ind_moins[card_donnee/2];
    int ind_plus[card_donnee/2];
    int moins = 0;
    int plus = 0;
    int* Paramsenfant = malloc(sizeof(int)*cardParam-1); //On va donner à chaque enfant les mêmes paramètres excepté celui pour split
    for (int i = 0; i<cardParam; i++)
    { 
        float temp = informationGain(indices,card_donnee,params[i],data);
        if (Gain_max<temp){
            Gain_max = temp;
            indgainmax = params[i];
        }
    }
    int separatrice = 0;
    for (int i =0; i<card_donnee; i++) //On calcule la moyenne du paramétre choisi pour le split 
    {    
        separatrice += data[indgainmax][i]/card_donnee;
    }
    for (int i = 0 ; i<card_donnee;i++) //On sépare l'ensemble considéré entre les valeurs supérieur et inférieur à la moyenne
        {
            printf("%d",card_donnee);
        if (data[indgainmax][indices[i]]<separatrice)
        {
            ind_moins[moins] = indices[i];
            moins++;
        }
        else
        {
            ind_plus[plus] = indices[i];
            plus++;
        }
    }
    Noeud* parent = creerNoeud(indices,separatrice,card_donnee,params,cardParam,hauteur);
    parent->paramSplit = indgainmax;
    int compteur = 0;
    for (int i = 0; i<cardParam;i++)
    {
        if(i != indgainmax){
            Paramsenfant[ compteur] = params[i];
            compteur++;
        }
    }
    Noeud* fg = creerNoeud(ind_moins,0.0,card_donnee/2,Paramsenfant,cardParam-1,hauteur+1);
    Noeud* fd = creerNoeud(ind_plus,0.0,card_donnee/2,Paramsenfant,cardParam-1,hauteur+1);
    parent ->droit = fd;
    parent ->gauche = fg;
    return parent;
}
Noeud* creaParcourNoeud(Noeud* racine, int prof_max,float data[MAX_ROWS][MAX_COLS])
{
    printf("\n ok-creerparcour");
    if (racine->hauteur == prof_max || racine->card_donnee == 1||racine->card_donnee==0) //si la profondeur max est atteinte ou si il n'y a plus assez de donnée on arréte de split
    {
        return racine;
    }
    else    //Sinon on a affecte a fils droit et fils gauche la valeur de leur split (split renvoie le Noeud dont les fils sont les deux ensembles obtenus)
    {   
        printf("\n card racine %d",racine->card_donnee);
        
        Noeud* fils = trouverPointdeSplit(racine->indParams,racine->index_donnee, racine->card_donnee, racine->cardParam, racine->hauteur,data);
        racine->gauche = fils->gauche;
        racine->droit = fils->droit;
        racine->gauche = creaParcourNoeud(racine->gauche, prof_max,data); //on applique ensuite récursivement le principe de création d'un arbre
        racine->droit = creaParcourNoeud(racine->droit,prof_max,data);
        free(fils);
        return racine;
    }
}

AbrDecision* creer_arbre(int* ensparam,int* indices,int cardParam,int cardIndex,int profondeur_max,float data[MAX_ROWS][MAX_COLS]){
    printf("\n ok-crearbre");
    Noeud* arbre = creerNoeud(indices,cardIndex,0,ensparam,cardParam,0);
    printf("%d",arbre->card_donnee);
    arbre = creaParcourNoeud(arbre,profondeur_max,data);
    AbrDecision* abr = malloc(sizeof(AbrDecision));
    abr->tree = malloc(sizeof(Noeud*));
    assert(abr->tree!= NULL);
    abr->min_samples_leaf= 5;
    abr->min_samples_split = 5;
    abr->profondeur_max = profondeur_max;
    abr-> tree = arbre ;
    return abr; 
}


float prediction(Noeud* abr,float Indices_test[],float data[MAX_COLS][MAX_ROWS]) {
    printf("\n ok-prediction");
    if (abr->gauche == NULL && abr->droit == NULL){ // si il n'y a pas d'enfant on renvoi la valeur de donnée de cette feuille
        return data[abr->index_donnee[0]][4];
    }
    else {
        if(Indices_test[abr->paramSplit]<= abr->seuil){ //sinon on parcour les enfants suivant la position du paramétre en question
            return prediction(abr->gauche,Indices_test,data);
        }
        else{
            return prediction(abr->droit,Indices_test,data);
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
    printf("\n ok-bootscore");
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
    printf("%c\n ok-bootstrap\n");
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
        foret->forest[j] = creer_arbre(Combi[j], bootstrapInd, foret->nbParamParArbre,oob, foret->max_depth,data);
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
    FILE*file;
    file = fopen("C:\\Users\\valen\\Desktop\\ProgrammesC\\Resultats.csv", "r");     //mettre le chemin d'accès au dossier CSV sur son ordi
    if (file == NULL) {
        perror("Impossible d'ouvrir le fichier");
        return 1;
    }

    char line[1024];                               //à modifier selon le nombre de ligne que l'on a 
    int rows = 0;
    int cols = 0;
    float data[MAX_ROWS][MAX_COLS]; // Matrice pour stocker les données
    printf(" data %d",data[75][75]);

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
    draw_bootstraprand(60,foret,data);
    // Predict using the trained RandomForest

    return 0;
}
