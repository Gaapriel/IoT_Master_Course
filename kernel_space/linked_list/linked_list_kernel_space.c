/*
 *  Opettaja : Erno Hentunen
 *
 *   Tehtävä : Linux kernel, Harjoitus 3 : Linked list and reserving memory
 *
 *      • Create a module with one module parameter: size of the linked list.
 *      • The nodes in the list will be structures with string (char* ) denoting the position
 *        of the node in the list (so each list member will print out “I am number $d on the list”
 *      • Init function will populate and print out the list
 *      • Exit function deletes the list.
 *
 * ---------------------------------------------------------------------------------------------------
 *
 *  Harjoitustyön tekijä : Erno Kilpeläinen (erno.kilpelainen@hotmail.com)  06.05.2017
 *
 *  Huomioitavaa :
 *
 *  - Saadun palautteen pohjalta, muutin kommentoinnin kotimaiselle kielelle.
 *
 *  - Linkitetystä listasta :
 *      - Linkittyn listan tekoon löytyy valmiita kirjastoja (linux/list.h).
 *      - Tein listan itse jotta oppii tekemään sen...
 *      - Linkitetty lista täytetään etupuolelle (nopein tapa), joten NULL solu siirtyy aina listalla.
 *      - Etupuolelta täytössä on myös se ilmiö että ensimäisenä lisätyt
 *        nodet siirtyvät taakseppäin. Joten jos for looppi on nouseva (kuten yleensä)
 *        tulostuu listan alkiot suurimmasta pienimpään.
 *      - Laitoin for loopin väärinpäin, helpoin tapa tulostaa listan solut nousevassa järjestyksessä
 *      - Solujen (alustus ja luonti) tapahtuu omassa function toimintokutsussa (LINKED_LIST_ACTION_POPULATE)
 *
 *  - Modulin latausparametrista :
 *      - Modulin latausparametrina on yksi int muuttuja
 *      - Latausparametri on rajattu välille 1 - 100 jottei tule mitään hullun pitkiä tulostuksia
 *      - Jos yrittää antaa vääriä parametreja niin siitä tulee virhe viesti logiin
 *
 *  - Logi tulostuksista :
 *      - Tulostetaan on pelkästään listan *data alkio (Linked list task : Im number %d in this linked list)
 *      - *data alkion tulostusprioriteetti on : KERN_INFO joten väliin saattaa tulla muita tulostuksia
 *      - Tulostus tapahtuu erillisessä function toimintokutsussa (LINKED_LIST_ACTION_PRINT)
 *      - Erilaiset virhetilanteet yms info tulostetaan erillisessä functiossa : linked_list_print_info
 *
 *  - Koodi on käännetty ja kokeiltu ubuntu 16.04.2 LTS (xenial), kerneli 4.8.0-49-generic
 *
 */ 

// Toiminnolliset määritteet
#define LINKED_LIST_ACTION_POPULATE 1
#define LINKED_LIST_ACTION_PRINT    2
#define LINKED_LIST_ACTION_CLEANUP  3

// Erilaisten tekstipuskurien koko
#define BUFF_SIZE   240

// Sovelluksen nimi
#define PROGRAM_INTERNAL_NAME "Linked list task"

// Lisätään tarvittavat kirjastot
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>

// Parametrien käsittely
static int param = 0;           // Muuttujan alustus
module_param(param, int, 0);    // Haku
// Parmetrin esittely (jos joku sattuu katsomaan moduli infosta (Komento : modinfo <modulin nimi>.ko))
MODULE_PARM_DESC(param, "Give number of printout. Example : param=22");

// Ohjelman sisäinen virhe-numero
static int p_error; 

// Prototyyppien esittely
void linked_list_print_info(int error);
int linked_list_operations(int operation);



static int __init linked_list_init(void) {

    // Kirjotetaan ohjelman aloitusteksti
    linked_list_print_info(1);

    // Laitetaan oletustilanne virheiden käsittelyyn
    p_error = 0;
    
    // Parametrien tarkistus
    if( param < 1 || param > 100 ) {
        p_error = -101;
        linked_list_print_info(p_error);
        return -1;   
    }

    // Laitetaan solut istalle
    if( linked_list_operations(LINKED_LIST_ACTION_POPULATE) < 0 )
        return -1; // Ei onnistunut, ei voida jatkaa
    
    // Tulostetaan lista (palauttaa aina onnistumisen)
    linked_list_operations(LINKED_LIST_ACTION_PRINT);

    // HUOM!  LINKED_LIST_ACTION_CLEANUP toimintoa kutsutaan __exit toiminnon yhteydessä.
    
    // Tulostetaan onnistunut poistuminen
    linked_list_print_info(p_error);
    return 0;

}



// Vapautetaan varatut resurssit
static void __exit linked_list_exit(void) {
 
    // Laitetaan funktio vapauttamaan varaamansa resurssit
    linked_list_operations(LINKED_LIST_ACTION_CLEANUP);
    linked_list_print_info(2);
}


// Tulostaa toiminto- ja virheviestin logiin printk-functiolla
// Positiiviset arvot on info tulosteita, negatiiviset virheitä
void linked_list_print_info(int error) {

    // Tutkitaan tulostettava viesti
    switch( error ) {
        case 2 :
            printk(KERN_INFO  "%s : Reserved resources released \n", PROGRAM_INTERNAL_NAME);
            break;         
        case 1 :    // Info viesti
            printk(KERN_INFO  "%s : Program started \n", PROGRAM_INTERNAL_NAME);
            break; 
        case 0 :    // Ei virhettä, vaan kirjoitetaan oletus poistumiseen liittyvät tiedot
            printk(KERN_INFO  "%s : Normal exit without error \n", PROGRAM_INTERNAL_NAME);
            break; 
        case -101 : //  Viestissä mukana myös virheellinen parametriarvo
            printk(KERN_ALERT "%s : Error code : %d (Invalid parameter = %d) Parameter must be postitive number between 1 - 100)\n", PROGRAM_INTERNAL_NAME, error, param);
            break; 
        case -102 :
            printk(KERN_ALERT "%s : Error code : %d  Can not kmalloc string buffer.\n",PROGRAM_INTERNAL_NAME, error);
            break;
        case -103 :
            printk(KERN_ALERT "%s : Error code : %d  Can not kmalloc first idx node.\n",PROGRAM_INTERNAL_NAME, error);
            break;
        case -104 :
            printk(KERN_ALERT "%s : Error code : %d  Can not kmalloc p node.\n",PROGRAM_INTERNAL_NAME, error);
            break;
        case -105 :
            printk(KERN_ALERT "%s : Error code : %d  snprintf cant add text to buff inside loop.\n",PROGRAM_INTERNAL_NAME, error);
            break;
        case -106 :
            printk(KERN_ALERT "%s : Error code : %d  kmalloc cant allocate memory for text in loop.\n",PROGRAM_INTERNAL_NAME, error);
            break;
        case -107 :
            printk(KERN_ALERT "%s : Error code : %d  snprintf cant add text to buff in loop.\n",PROGRAM_INTERNAL_NAME, error);
            break;
        default :
            printk(KERN_ALERT "%s : Error code : %d  Un-defined error \n",PROGRAM_INTERNAL_NAME, error);
            break;
    } // switch : error
}


/*
 * Ohjelman toiminnallinen functio :
 *
 * Kutsutaan :
 *  - linked_list_init
 *    Parametreilla :
 *     - LINKED_LIST_ACTION_POPULATE
 *     - LINKED_LIST_ACTION_PRINT
 *  - linked_list_exit
 *    Parametreilla :
 *     - LINKED_LIST_ACTION_CLEANUP
 *
 * Parametrien toiminnallisuus :
 *  - LINKED_LIST_ACTION_POPULATE
 *    - Luo ja täyttää linkitetyn listan (lataus parametrissa määritellyn solumäärän mukaisesti)
 *  - LINKED_LIST_ACTION_PRINT
 *    - Tulostaa printk-funktiolla listan koko sisällön
 *  - LINKED_LIST_ACTION_CLEANUP
 *    - Vapauttaa varatut resurssit
 *
 * Palautusarvo :
 *  - < 0 = Virhe (numero kertoo mikä virhe)
 *  -   0 = Toiminto onnistui
 *
 * Virheiden käsittely :
 *  - Virhetilanne luo virhenumeron p_error muuttujaan
 *  - Siirrytään virheen käsittelyyn goto tominnolla : error_handler :
 *  - Virheen tulostus : linked_list_print_info(p_error);
 *  - Palauttaa virhenumeron return parametrina
 *
 */
int linked_list_operations(int operation) {

    // Tehdään tructi tallennettavalle tiedolle, nimetään se muuttujatyypiksi node_s
    typedef struct node {
        char *data;
        struct node *next;
    } node_s;

    // Muuttujat joiden resurssia ei tarvi erikseen vapauttaa
    int buff_s = 0;  // Muuttuja kirjoitetuille merkeille (käyttään myös muistin varaukseen)
    int loop = 0;    // Silmukan muuttuja
    
    // Staattiset muuttujat, muistivaraukset vapautetaan 
    static node_s *p, *idx; // Määritellään osoittimet indeksille ja uudelle solulle
    static char *buff;      // Tekstipuskurin osoitin (staattinen jotta vapautus onnistuu keskitetysti, esim eri virhetilanteissa)

    // Katsotaan mitä halutaan tehdä
    switch( operation ) {

        // Alustetaan ja täytetään linkitetty lista
        case LINKED_LIST_ACTION_POPULATE :

            // Varataan muistia tekstipuskurille, GFP_KERNEL prioriteetilla
            buff = kmalloc( sizeof(char) * BUFF_SIZE, GFP_KERNEL );
            if( buff == NULL ) {
                p_error = -102;
                goto error_handler;
            }

            // Varataan tilaa aloitus solulle
            idx = kmalloc( sizeof(node_s), GFP_KERNEL );
            if( idx == NULL ) {
                p_error = -103;
                goto error_handler;
            }
            
            // Aloituspaikan osoite (muut solut lisätään tämän etupuolelle, joten tämä pysyy listan lopussa)
            idx = NULL;

            // Linkitetyn listan solujen luonti-silmukka (silmukka laskevassa järjestyksessä)
            for( loop = param; loop > 0; loop--) {
                
                // Muistin varaus ja tarkistus uudelle nodelle (listan solulle)
                p = kmalloc( sizeof(node_s), GFP_KERNEL );
                if( p == NULL ) {
                    p_error = -104;
                    goto error_handler;
                }

                // Tallennetaan tulostettava rivi snprintf alustaa kirjoituspuskurin joten memset yms. alustuskomentoja ei tarvita
                buff_s = snprintf(buff, BUFF_SIZE, "Im number %d in this linked list", loop);
                // Tarkistetaan
                if( buff_s < 0) {
                    p_error = -105;
                    goto error_handler;
                }

                // snprintf-funktio palauttaa kirjoitetun tietomäärän yhtä merkkiä lyhempänä.. en kerinnut setviä miksi..
                // Jos ei tässä kasvata kirjoitettujen merkkien märää, niin viimeinen merkki tekstistä jää pois
                buff_s++;

                // Varataan muistia kirjoitettavalle tekstille (varataan tilaa myös \0 merkille)
                p->data = kmalloc( sizeof(char) * (buff_s + 1), GFP_KERNEL);
                // Tarkistetaan
                if( p->data == NULL ) {
                    p_error = -106;
                    goto error_handler;
                } 
                
                // Kopioidaan viestin SISÄLTÖ muuttujaan (ei osoitetta, p->data = buff).
                buff_s = snprintf(p->data, buff_s, "%s", buff);
                // Tarkistetaan
                if( buff_s < 0 ) {
                    p_error = -107;
                    goto error_handler;
                }

                // Asetetaan listan jatko-kohdaksi nykyinen indeksi (ensimäisellä kerralla tulee NULL > listan loppu)
                p->next = idx;

                // Tallennetaan uusi indeksikohta listassa
                idx = p;

            } // for : loop

            break;

        // Tulostetaan solut
        case LINKED_LIST_ACTION_PRINT :

            // Asetetaan aloitusindeksi
            p = idx;
            
            // Tulostetaan lista
            while(p != NULL) {
                printk(KERN_INFO "%s : %s\n",PROGRAM_INTERNAL_NAME, p->data);
                p = p->next;
            }

            break;

        // Vapautetaan varatut resurssit
        case LINKED_LIST_ACTION_CLEANUP :   // Varatun muistin tyhjennys
    
            // Tutkitaan virhekoodeista, mitä varauksia pitää tyhjentää
        
            // Pelkkä tekstipuskurin vapautus
            if( p_error == -103 ) {
                kfree(buff);
            }
            // Vapautetaan kaikki varatut resurssit
            else {

                // Linkitetty lista
                if(idx != NULL) {
                    p = idx;
                    idx = idx->next;
                    kfree(p->data);     // Vapautetaan char *data varaama muisti
                    kfree(p);           // Vapautetaan osoitin
                }
                kfree(idx);     // Vapautetaan osoitin
                kfree(buff);    // Vapautetaan puskurin viemä muisti
            }

            break;

    } // switch : operation

    return 0;

    // Virheiden käsittely
    error_handler :
        
        linked_list_print_info(p_error);
        return p_error;

}


module_init(linked_list_init);
module_exit(linked_list_exit);

// License and author
MODULE_LICENSE("GPL");
MODULE_AUTHOR("erno.kilpelainen@hotmail.com");
MODULE_DESCRIPTION("Linked list task : School practice work (linked_list_kernel_space.c) 06.05.2017");

