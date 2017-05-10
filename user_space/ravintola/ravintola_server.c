/*
Opettaja : Erno Hentunen

Tehtävä :

Ravintola step 1
- Tehdään ravintolan tilauspalvelu, joka toimii internetin yli.
- Luo socket, joka odottaa esim. portissa 5000 yhteyksiä. Kun yhteys on luoto, forkkaa lapsi ja anna uus yhdistetty 
  socket (accept():n paluuarvo) sille. Lapsi hoitaa yhteyden clienttiin siten, että se lukee mitä clientillä on sanottavaa ja 
  kirjottaa sen ulos. Kun read()-funktion paluuarvo on 0, sulje connection socket ja tapa lapsi.
- Nyt siis voi olla useampia clienttejä kiinni yhtä aikaa. Clientiksi kelpaa vaikka ”echo”-ohjelma tai 
  voit kirjoittaa yksinkertaisen clientin.
 
Ravintola step 2
- Lisää pääohjelmalle alkuun yksi lapsi forkkaamalla.
- Tästä tulee se varsinainen tilausten käsittelijä. Tämä lapsi luo fifon kuten fifo esimerkissä eli fifo suljetaan ja avataan 
  jokaisen viestin jälkeen ja luettu viesti tulostetaan.
- Muuta client handlerit (eli lapset jotka saavat connection socketin) printf:n tilalta kirjoittamaan fifoon.

------------------------------------------------------------------------------------------------------------------

Harjoitustyön tekijä : Erno Kilpeläinen (erno.kilpelainen@hotmail.com)

Huomioitavaa :
- Lisätty runsaasti kommentteja itseä ja opiskelija tovereita silmällä pitäen.
- Kommentoinnin takia lisätty muuttujia funktioiden palautusarvoille (muutoi olisi vertailtu suoraan if-lauseissa),
  sekä hajoitettu funktio kutsujen parametreja useammalle riville, jotta jokainen parametri voidaan kommentoida.
- Ohjelmaan lisätty "debug" tulostuksia, jotta prosesseja on helpompi seurata.
  DEBUG_MESSAGES = 0 (ei debug viestejä) DEBUG_MESSAGES = 1 (viestit mukana)

Ohjelman osat : (tarkemmat selityksen kunkin ohjelman osan yhteydessä)
- main : 
  Ohjelma käynnistyy mainiin, mainin tehtävä on alustaa ohjelma, herättää henkiin tarvittavat lapsiprosessit.
  Ajon aikana main vastaanottaa uudet yhteydet ja luo tilausten vastaanotto prosessin (Order_Process) jokaiselle yhteydelle.
- Order_Handle_Process- funktio (vain yksi kappale) :
  Luodaan ohjelman alustuksen yhteydessä. Lukee FIFO puskuria ja tulostaa näytölle FIFO puskurin kautta tulleet tilaukset.
- Order_Process- funktio (luodaan jokaiselle yhteydelle erikseen) :
  Luodaan jokaiselle yhteydelle. Voi olla useita kappaleita olemassa samaan aikaan. Kommunikoi Socket:n kautta
  asiakas clienttiin ja lähettää saamansa datan FIFO puskuriin.
- print_debug :
  Tulostaa debug viestit näytölle
- error :
  Tulostaa virhe-ilmoitukset ja lähettää lopetusviestin kaikille prosesseille
- sigHandler :
  Käsittelee saapuneet signaalit. Lähettää ohjelman lopetus signaalin prosesseille ja käsittelee SIGCHLD signaalin
  waitpid- toiminnolla.


Sudenkuoppia (joiden kanssa sai taistella) :
- Order_Handle_Process (joka lukee FIFO tiedostoa, ei kirjoita sinne) jäi loputtomaan lukusilmukkaan palauttaen aina 0:n
  FIFO tiedoston avaus pelkästään luku tilaan aiheutti loppumattoman loopin luku-rutiinissa.
  Ratkaisu :
  Helpoin tapa ohittaa k.o ongelma oli laittaa tiedoston avaus luku ja kirjoitus tilaan jolloin
  read- funktio jäi kiltisti odottamaan kunnes FIFO tiedostossa oli uutta luettavaa.
- Suljettujen child prosessien varaamat resurssit (lähinnä PID numerot).
  Kun child prosessi suljetaan, ei se näköjään vapauta käyttöjärjestelmästä PID numeroa vaan jää roikkumaan zombieksi.
  Tästä aiheutuu ongelmia kun tilauksia tuli vauhdilla (>5 kpl sekunnissa). Käyttöjärjestelmään jäi roikkumaan niin paljon
  suljettuja child prosesseja ja näiden varaamia PID numeroita että käyttöjärjestelmä ilmoitti systeemi virheellä ettei
  PID palvelu ole hetkellisesti käytössä.
  Ratkaisu :
   Käsittely tapahtuu waitpid- funktiolla joka parametroidaan kuuntelemaan kaikkia ohjelman child- prosesseja.
   wautpid- funktiossa käytetään WNOHANG parametria jolloin ohjelma ei jää jumittamaan jos lapsi ei ole vielä valmis.
   Linuxi tuntui hermostuvan jos waitpid funktiota kutsutaan child-prosessin alas-ajo signaalin (SIGCHLD) aktivoiduttua.
   Siirretty waitpid-funktio signaalin luku funktiosta, pääohjelman silmukkaan.

Tiedossa olevia bugeja :
- Jos viesti (tilaus) on suurempi kuin puskurin koko -> Tulee useammalla lukukerralla,
  tulkitsee tilauksen tulostus rutiini jatkoviestit uudeksi tilaukseksi.
- Linux puolella ajettaessa pitää lisätä viivettä main ohjelmaan Order_Process forkkauksen jälkeiseen kohtaan jossa 
  main prosessin puolelta suljetaan newsockfd (samainen newsockfd on annettu child prosessille parametrina)
  Tämä onglma saattaa myös johtua, että itse ajan linuxia VirtualBox:n sisällä joten suorituskyky ei ole paras mahdollinen,
  tätä voi testailla muuttamalla ORD_PR_FORK_DELAY_MS vakion arvoa.

Kehityskohteita :
- Laittaa ohjelman toimintaan liittyviä asioita käynnistys parametreiksi :
  - Osoite ja portti mitä kuunnellaan
  - Mahdollinen viive Order_Process forkkauksen jälkeen
  - Debuggi tulostukset on/off
  - Lisäviestien odotuksen keskeyttävän, terminointi merkin määritys
- main ja Order_Handle_Process prosessien lopetuksen yhteydessä oleva viive. 
  Viiveen tarkoitus on odottaa muiden prosessien loppumista (ettei esim. poisteta tiedostoja liian aikaisin).
  Tämän viiveen tilalle pitää laittaa toiminto joka odottaa muiden prosessien sammumista pid numerolla.
  Yksi mahdollisuus on tutkia ohjelman lopetuksen aikana signaalin käsittelyn waitpid-funktion palautusarvoa,
  jos lopetus on päällä, ja kaikki child prosessit kuolleet -> jatketaan sammutusta.

Ohjelman testaus :
- En ole tehnyt varsinaista tilaus-client ohjelmaa testausta varten.
- Ohjelmaa voi testata esim. echo komennolla seuraavasti (linux, macOS) :
  echo -n "viesti" | nc <ipaddress> <port>  (esim. echo -n "hello and bye" | nc localhost 5000)
- Samassa hakemistossa on yksinkertainen bash-scripti (bash_client) jolla voi pommittaa tilauksia tulemaan.
  Parametrit : "viesti" <viive> <osoite>, Viesti : Lähetettävä tilausviesti (vapaata tekstiä), Viive : Viestien
  lähetysviive sekunteina, Osoite : ip osoite jonne viestejä lähetetään.
  Esim. ./bash_client "viesti" 4 localhost. bash_client lisää viestin loppuun seuraavassa kohdassa selitetyn 
  TERMINATE_CHAR-merkin.
- Ohjelma odottaa uusia merkkejä Socket:ilta (MAX_RETRY_COUT) * (DELAY_BW_RETRYES_MS) ajan. Tämän voi ohittaa
  laittamalla tilauksen viimeiseksi merkiksi TERMINATE_CHAR muuttujassa määritelty merkin '\b', tällöin ohjelma 
  tulkitsee ettei uusia merkkejä tule ja poistuu Socket luku rutiinista (bash_client lisää terminointi merkin).

Kun laittaa 10 ikkunaa lähettämään bash_client tilauksia, 0 sekunnin viiveellä, saa mukavasti vipinää aikaiseksi.

Ohjelman sammuttaminen :
- Lähettämällä palvelimelle tilauksen : quit
- Painamalla ctrl c (ctrl c aiheuttaa keskeytys signaalin lähettämisen kaikille ohjelman prosesseille)
- Virhe tilanne. Virheiden käsittelyrutiini (error) lähettää ryhmäviestin SIGQUIT kaikille
  prosesseille joka aiheuttaa "hallitun" alas ajon jokaisen prosessin sisällä. Testattu mm. poistamalla fifo
  tiedosto kesken kaiken...
- Ohjelman voi sammuttaa myös tappamalla jomman kumman pysyvistä prosesseista KILL komennolla (main ja Order_Handle_Process).
  Molemmat prosessit lähettävät ryhmäviestin SIGQUIT muille prosesseille jotta tietäävät sammuttaa itsensä.
  Prosessit näkyvät samalla nimellä, mutta yleensä pienemmällä PID numerolla oleva prosessi on main.


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>

// Vakioiden määrittelyä
#if __APPLE__
 #define ORD_PR_FORK_DELAY_MS 1                 // Viive (ms) jos ajetaan aplella (mädällä omenalla)
#else
 #define ORD_PR_FORK_DELAY_MS 50                // Viive (ms) muissa käyttöjärjestelmissä,
#endif                                          // Oma VirtualBox:ss toimiva linux vaati vähintään 50 ms viiveen

#define DEBUG_MESSAGES  1
#define SERV_PORT       5000                    // Portti jota palvelin kuuntelee
#define BUFF_SIZE       255                     // Luku/kirjoitus puskurien koko
#define FIFO_FILE       "/tmp/ravintola_fifo"   // FIFO tiestoston polku/nimi

// Globaalit muuttujat
static int close_program;          // 1 = Ohjelman lopetus signaali saapui
static pid_t main_pid;               // Pääohjelman PID jolle error funktio lähettää keskeytyspyynnön (ei tarvi kuljettaa funktioparametrina)

// Funktioiden prototyypit
void error(char *msg);                          // Yksinkertainen virheiden tulostus
void Order_Process(int cli_fd);                 // Jokaiselle erilliselle tilaukselle forkattava child. Parametrina muodostetun yhteyden Filedescriptori
void Order_Handle_Process(void);                // Child joka käsittelee kaikki tilaukset ja tekee tulostuksen
void print_debug(char *msg, pid_t pid);         // Statustietojen tulostus
static void sigHandler(int sig);                // Signaalin käsittely


/*
 * ---------------------------------------
 *    Ravintola- sovelluksen pääohjelma
 *                main
 * ---------------------------------------
 *
 * Luonti / Käynnistys :
 * - Ohjelman käynnistyspiste
 *
 * Parametrit :
 * - Parametreja ei tulkita
 *
 * Palautusarvot :
 * - Palauttaa 0:n
 *
 * Tehtävät :
 *  - Asettaa seurattavat signaalit (SIGQUIT, SIGINT, SIGTERM, SIGCHLD)
 *  - Luodaan (fork) tilausten hallinta prosessi (Order_Handle_Process)
 *  - Alustaa ja asettaa Socketin ja Bindauksen
 *  - Asettaa Socketin kuuntelu tilaan (listen)
 *  - Mennään toimintalooppiin jossa seuraavat toimenpiteet :
 *     - Odottaa acceptissa uutta yhteyttä
 *     - Luodaan (fork) tilauksen vastaanotto prosessi (Order_Process), annetaan parametriksi socketin FD
 *     - Palataan odottamaan uutta yhteyttä
 *     - Loopista poistutaan jos ohjelma ajetaan alas tai joku virhe on tapahtunut
 *  - Suljetaan avoimet File Descriptorit
 * 
 * Poistuminen/Sulkeminen :
 * - Poistutaan vain kun ohjelma ajetaan alas (ctrl c, quit viesti tai virhe)
 * - Ohjelman sulkeminen exit komennolla 
 *
*/
int main(int argc, char *argv[]) {

    #define SERV_MAX_WCONS  5                   // Palvelimen odottavien yhteyksien puskurin koko   

                int         connCounter;        // Montako yhteyttä muodostettu 
                int         sockfd;             // File descriptor serverin socketille
                int         newsockfd;          // File descriptor muodostetulle uudelle yhteydelle
                int         retValue;           // Eri toimintojen palautusarvo
    unsigned    int         clilen;             // Muodostetun client yhteyden osoitetietojen pituus
    struct      sockaddr_in serv_addr;          // Osoite informaatio struckti palvelimen asetuksille (in.h)
    struct      sockaddr_in cli_addr;           // Osoite informaatio struckti muodostetulle client yhteydelle (in.h)
                pid_t       oh_child_pid;       // Tilausten käsittelijän (Order_Handle_Process) PID > Luodaan vain kerran
                pid_t       co_child_pid;       // Tilausten vastaanottajan (Order_Process) PID > Luodaan joka socket viestille
                char        dmsg_buff[BUFF_SIZE];   // Debug viestin kokoamista varten
    struct      sigaction   sa;
                int waitPidStat;

    // Nollataan laskureita
    close_program = connCounter = 0;

    // Haetaan prosessitunnus (debug printtauksia varten)
    main_pid = getpid();

    if( DEBUG_MESSAGES ) {
        // Yksinkertainen ostikko, myöhempiä debug tulostuksia varten
        printf("\n  PID       PROCESS                 MESSAGE\n");
        printf("-----------------------------------------------------------------------------\n");
        fflush(stdout); // Pakotetaan terminaali tulostamaan otsikkotekstit
    }
    else
        printf("\nRunning without debug messages...\n");

    print_debug("Server init (main)   : Preparing signal handler (sigHandler) ", main_pid);

    // Muodostetaan signaali käsittelijä
    sa.sa_handler = sigHandler;     // Signaalin käsittly funktion nimi
    sigemptyset(&sa.sa_mask);       // Nollataan vastaanotettavat signaalimääritykset (alempana määritellään halutut signaalit)
    sa.sa_flags = 0;

    // Asetellaan erilaisia keskeytykseen liittyviä signaaleita seurantaan, jotta osataan ajaa ohjelma alas hallitusti,
    // reagoi esim ctrl c komentoon.
    // Signaalit löytyy : signal.h joka useimmiten viittaa sys/signal.h
    if( sigaction(SIGQUIT, &sa, NULL) == -1 )   // SIGQUIT : Ohjelman lopetus (esim. quit tilaus-viesti tekee tämän)
        error("Problem installing signal handler SIGQUIT (main)");

    if( sigaction(SIGINT, &sa, NULL) == -1 )    // SIGINT : Ohjelman keskeytys (esim ctrl c) 
        error("Problem installing signal handler SIGINT (main)");

    if( sigaction(SIGTERM, &sa, NULL) == -1 )   // SIGTERM : Terminate signaali (esim. terminaalin kill)
        error("Problem installing signal handler SIGTERM (main)");

    // Luodaan child joka käsittelee tilaukset (Order_Handle_Process)
    oh_child_pid = fork();  

    if( oh_child_pid == 0 ) {
        print_debug("Server init (main)   : Making order handling (Order_Handle_Process) ", main_pid);
        Order_Handle_Process();
        return 0;
    } 
    else if( oh_child_pid == -1 )
        error("ERROR on order handling process child forking (main)");
    else
        // Odotellaan hetki että tilausten käsittelijä prosessi pääsee kunnolla pystyyn
        // Ei mennä ottamaan tilauksia vastaan ennen aikojaan...
        sleep(1);

    print_debug("Server init (main)   : Making socket handler", main_pid);

    // Avataan socket. Palauttaa file descriptorin jos onnistu, -1 jos ei onnistunut, virhe löytyy errno muuttujasta
    sockfd = socket(            // socket.h
                AF_INET,        // domain (socket.h) : AF_INET = ipv 4, AF_INET6 = ipv6, AF_IPX jne.
                SOCK_STREAM,    // type (socket.h)   : SOCK_STREAM = 2-suuntainen yhteys (TCP), SOCK_DGRAM = 1-suuntainen yhteys (UPD), SOCK_RAW (IP)
                IPPROTO_IP);    // protocol (in.h)   : IPPROTO_IP = Käyttöjärjestelmä valitsee, IPPROTO_UDP, IPPROTO_TCP jne...

    // Virheen tarkistus
    if( sockfd < 0 ) 
        error("ERROR opening socket (main)");

    // Nollataan/kirjoitetaan yli serv_addr muuttuja (muuten siellä voi olla mitä tahansa merkkejä)
    memset(                     // string.h
        (char *) &serv_addr,    // Osoitin serv_addr muuttujaan
        0,                      // Kirjoitetettava merkki (int tyyppiä)
        sizeof(serv_addr));     // Kirjoitettavan tiedon määrä 

    // Palvelimen osoitteen määritykset
    serv_addr.sin_family        = AF_INET;            // Osoiteperhe (socket.h) : internet = UPD, TCP jne.
    serv_addr.sin_addr.s_addr   = htonl(INADDR_ANY);  // Osoiteavaruus (in.h)   : INADDR_LOOPBACK = 127.0.0.1 (localhost), INADDR_ANY = Kaikki
    serv_addr.sin_port          = htons(SERV_PORT);   // Portti (in.h/types.h)  : <numero> (htons() muuntaa sen käyttöjärjestelmän hyväksymään muotoon.


    // Yhdistetään/liitetään varattu socketti (sockfd), serverin osoiteeseen (struckti pointteri)
    // Palauttaa 0 jos onnistui, -1 jos ei onnistunut, virhe löytyy errno muuttujasta
    retValue = bind(                            // socket.h
                sockfd,                         // Filedescriptori palvelimen socketille
                (struct sockaddr *) &serv_addr, // Strutct osoitin palvelimen serv_addr muuttujaan. 
                sizeof(serv_addr));             // serv_addr muuttujan koko (Palvelimen osoite structi) 

    // Palautusarvon tarkistus
    if( retValue < 0) 
        error("ERROR on binding (main)");

    // Kuunnellaan/odotetaan socket yhteyttä. Palautta 0 jos onnistui -1 jos ei onnistunut, virhenumero errno muuttujassa
    retValue = listen(                  // socket.h
                    sockfd,             // Socketin Filedescriptori (int) (bindattu ylempänä)
                    SERV_MAX_WCONS);    // Backlock : Odottavien yhteyksien maksimi määrä (SOMAXCONN)
    // Tarkistetaan
    if( retValue < 0 )
        error("ERROR on listen (main)");
    
    // Client yhteyden osoitemäärityksien kokovaraus
    clilen = sizeof(cli_addr);
  
    // Serverin kuuntelu iki-looppi (loopi keskeytetään ctrl c tai lähettämällä quit "ravintola" tilauksena)
    // Looppi keskeytyy heti (eikä edes mennä) jos ohjelmaa ollaan ajamassa alas
    for(;close_program == 0;) {


        print_debug("Socket Server (main) : Waiting connections from clients", main_pid);

        // Yhteyden hyväksyntä. Jos onnistuu, palauttaa Filedescriptorin Client yhteydelle, < 0 virhe, virhenumero errno muuttujassa
        //  - accept kirjoittaa clientin osoitetiedot cli_addr muuttujaan
        //  - accept kirjoittaa todellisen cli_ddr muuttujan pituuden clilen muuttujaan
        newsockfd = accept(                             // socket.h
                        sockfd,                         // Socketin Filedescriptori
                        (struct sockaddr *) &cli_addr,  // Osoitin clientin cli_addr muuttujaan (struct tyyppinen muuttuja)
                        &clilen);                       // Osoitin clientin (cli_addr) koko muuttujaan 
    
        // Tutkitaan virhe
        if( newsockfd < 0 )
            error("ERROR on accept (main)");

        // Tutkitaan tuliko lopetuskomento (main ohjelma odottaa suurimman osan aikaa accept:ssa)
        if( close_program ) {
            print_debug("Socket Server (main) : Terminating by signal", main_pid);
            break;
        }

        // Nollataan debug viesti
        memset(dmsg_buff, 0, sizeof(dmsg_buff));

        // Kasataan debug viesti jossa mukana clientin ip numero (sin_addr) ja portti (sin_port)
        snprintf(dmsg_buff, sizeof(dmsg_buff), "Socket Server (main) : New connection accept from %s:%d", 
            inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
        
        // Tulosetaan debug viesti
        print_debug(dmsg_buff, main_pid);

        connCounter++;
        
        print_debug("Socket Server (main) : Make child (Order_Process) to handle Socket", main_pid);

        // Luodaan uusi lapsi prosessi hoitamaan viestin socket lukua, viedään uusi socketin filedescriptori (newsocfd) parametrina
        co_child_pid = fork();
        if( co_child_pid == 0 ) {
            Order_Process( newsockfd );
            return 0;
        } 
        else if( co_child_pid == -1 )
            error("ERROR forking Order_Process (main)");
        else
            // Suljetaan main prosessin newsockfd muuttuja, juuri forkatulla child prosessilla on oma kopio k.o muuttujasta
            usleep(ORD_PR_FORK_DELAY_MS * 1000);  // Piti laittaa viivettä, linuxin puolella antoi muuten virheilmoituksen
            close(newsockfd);

        // Ajetaan lapsi prosessit alas (ei jäädä jumittamaan vaan ajetaan kun child prosess on valmiina)
        while( 
          waitpid(        // waitpid palauttaa käsitellyn childin PID:n. 0 = Ei käsiteltävää, < 0 on vikatilanne
            WAIT_MYPGRP,  // Otetaan käsiteltäväksi mikä tahansa tämän ohjelman terminoitu child prosessi
            &waitPidStat, // Tänne tallentuu terminoinnin tilatietoja (ei tällä-erää käytetä)
            WNOHANG)
            > 0) { }; 

    }

    // Lähetetään lopetus-signaali kaikille lapsille
    killpg(getpgid(main_pid), SIGQUIT);

    // Odotetaan jotta prosessit sammuvat
    // Quick and dirty menetelmä, oikeasti tänne pitäisi laittaa kyttäys että kaikki avatut tilaus prosessit ovat sulkeutuneet (PID vahti)
    sleep(4);

    print_debug("Socket Server (main) : Terminated", main_pid);

    // Suljetaan avoimet File Descriptorit
    close(sockfd);
    close(newsockfd);
    exit(0);
}

/*
 * ---------------------------------------
 * Yksittäisen tilauksen käsittelyprosessi
 *             Order_Process
 * ---------------------------------------
 *
 * Luonti / Käynnistys :
 * - main prosessi luo jokaiselle onnistuneelle socket yhteydelle uuden Order_Process child prosessin
 * - Käynnistyy ilman viiveitä
 * - Voi olla useampia käynnissä yhtäaikaa, lukemassa eri portteihin ohjattuja Socket yhteyksiä
 *
 * Parametrit :
 * - cli_fd (int), Uuden main prosessissa hyväksytyn Socket yhteyden File Descriptori
 *
 * Palautusarvot :
 * - Ei palautusarvoja
 *
 * Tehtävät :
 *  - Avataan FIFO tiedosto (tarkistetaan onnistuiko)
 *  - Mennään toimintalooppiin jossa seuraavat toimenpiteet :
 *     - Nollataan lukupuskuri (buffer)
 *     - Luetaan Socketin muodostamasta File descriptorista tietoja (Parametrina saatu : cli_fd)
 *     - Kirjoitetaan luettu tieto FIFO puskuriin Order_Handle_Process- prosessille
 *     - Loopista poistutaan kun ei ole enää luettavaa tai jokin meni mönkään
 *       Ohjelma yrittää lukea Socket puskuria MAX_RETRY_COUT verran DELAY_BW_RETRYES_MS viiveillä.
 *       Retry silmukan voi ohittaa lähettämällä tilauksen merkkjonon viimeisenä merkkinä TERMINATE_CHAR 
 *       vakiossa määritelty merkki esim. \b
 * - Suljetaan avoimet FD:t (File Descriptorit)
 *
 * Poistuminen/Sulkeminen :
 * - Poistutaan heti kun Socket puskuri on tyhjä ja kaikki tiedot kirjoitettu FIFO tiedostoon tai virhetilanteessa
 * - Poistutaan _exit komennolla (ei aiheuteta ylimääräistä flush toimintaa muille prosesseille) 
 *
*/
void Order_Process(int cli_fd) {

    // Alla olevia vakioita muuttamalla vaikutetaan kuin kauan clientiltä yritetään lukea tietoja ja pidetään yhteyttä auki
    // Jos datan syöttäjä on ihminen (näppäimistöllä) pitää lukukertojen välinen viive olla tarpeeksi pitkä.
    #define MAX_RETRY_COUT          3       // Montako lukuyritystä kun Socket data on tyhjä (jos vaikka tulisi lisää dataa)
    #define DELAY_BW_RETRYES_MS     300     // Lukukertojen välinen tauko (ms)
    #define TERMINATE_CHAR          '\b'    // Merkki joka terminoi yhteyden, merkkijonon viimeinen merkki (ei mene retry silmukkaan)

    char    buffer[BUFF_SIZE];  // Puskuri tiedon lukemista varten
    int     n1, n2;             // Luettujen merkkien määrä
    pid_t   co_pid;             // Oma prosessitunnus
    int     fifo_fd;            // FIFO puskurin File descriptori
    int     soc_read_ret_count; // Socket:sta lukujen kerta

    // Haetaan prosessitunnus (debug printtauksia varten)
    co_pid = getpid();

    print_debug("Order_Process        : Open FIFO file for write", co_pid);

    // Avataan FIFO tiedosto, kirjoitustilaan. Palauttaa File descriptorin jos onnistuu
    fifo_fd = open(         // Tiedoton avaus (fntl.h)
                FIFO_FILE,  // Tiedoston nimi
                O_WRONLY);  // Avaus (fcntl.h) : Vain kirjoitustilaan
    
    // Avauksen tarkistus
    if( fifo_fd < 0 )
        error("ERROR Cant open FIFO file for write (Order_Process)");

    // Nollataan lukukertojen määrä
    soc_read_ret_count = 0;

    n1 = 1; // Laitetaan muuttuja päälle jotta alla oleva looppi aktivoituu

    // Toimintalooppi, poistutaan kun kaikki merkit on luettu tai ohjelmaa ajetaan alas
    for(;close_program == 0;) {

        // Nollataan puskuri
        memset(                 // string.h
            buffer,             // Nollattava muuttuja
            0,                  // Kirjotettava merkki
            sizeof(buffer));    // Paljonko merkkejä kirjotetaan

        // Luetaan Socketin (cli_fd) File descriptorista tietoja buffer muuttujaan. Palauttaa luettujen merkkien määrän
        n1 = read(              // unistd.h
                cli_fd,         // Käsiteltäväksi saadun clientin File descriptori
                buffer,         // Puskuri minne luetaan
                sizeof(buffer));// Montako merkkiä halutaan lukea (SSIZE_MAX määrittelee puskurin maksimin)

        // Tutkitaan virhe
        if( n1 < 0 )
            error("ERROR reading from socket (Order_Process)");
 
        // Jos Socket puskurista ei tule viestejä, tutkitaan terminointi
        else if( n1 == 0 ) {
            // Lisätään retry kertojen laskuria
            soc_read_ret_count++;

            if( soc_read_ret_count > MAX_RETRY_COUT ) {
                print_debug("Order_Process        : No more Socket data. Terminating", co_pid);
                break;
            }
            else
            {
                print_debug("Order_Process        : Waiting more Socket data", co_pid);
                usleep(DELAY_BW_RETRYES_MS * 1000);
            }
        }
        // Jos viestipuskuriin saatiin merkkejä, kirjoitetaan ne FIFO puskuriin
        // HUOM! Puskuria (buffer) ei nollata, vaan kirjoitetaan sellaisenaan sama määrä merkkejä jotka socket:sta luettiin (n1)
        else if( n1 > 0 ) {

            // Tukitaan oliko viimeinen merkitsevä merkki yhteyden terminointimerkki.
            if( buffer[n1-1] == TERMINATE_CHAR ) {
                // Asetetaan retry laskuri maksimille joten ei mennä retry looppiin
                print_debug("Order_Process        : Connection Terminate char found.", co_pid);
                soc_read_ret_count = MAX_RETRY_COUT;
            }
            else
                // Nollataan retry laskuri
                soc_read_ret_count = 0;
            
            print_debug("Order_Process        : Write socket data to FIFO file", co_pid);
 
            // Kirjotetaan luettu tieto sellaisenaan FIFO tiedostoon
            // Erikseen ei tarkisteta enää onko FIFO tiedostoa olemassa, jos FIFO on hävinnyt avauksen jälkeen, poistutaan virheellä
            n2 = write(         // unistd.h
                    fifo_fd,    // FIFO Filedeskcriptori
                    buffer,     // Socket:sta luettu tieto
                    n1);        // Socket:sta luettujen merkkien määrä

            // Tarkistetaan
            if( n2 < 0)
                error("ERROR wite to FIFO file (Order_Process");

        } // if : n1
    } // for : ;;

    print_debug("Order_Process        : Process terminated", co_pid);   

    // Suljetaan avoimet tiedostot. Close myös tyhjentää tiedostopuskurit joten data kirjoitetaan oikeasti levylle.
    //   Close funktiolla on palautusarvot, tässä tapauksessa closen palautusarvoja ei lueta
    //   (ei ole niin kriittistä tietoa etteikö se saisi hävitä jos close ei tomikkaan)
    close(fifo_fd);
    close(cli_fd);
    _exit(0);   // Suljetaan child (_exit sulkee ilman että aiheuttaa muiden prosessien FLUSH toimintoa)

}

/*
 * ---------------------------------------
 *  Kaikkien tilausten tulostus toiminto
 *        Order_Handle_Process
 * ---------------------------------------
 *
 * Luonti / Käynnistys :
 * - Main ohjelma fork:kaa ja käynnistää Order_Handle_Process:n heti alkuvaiheessa -> odottamaan tilaus prosesseja
 *
 * Parametrit :
 * - Ei käynnistysparametreja
 *
 * Palautusarvot :
 * - Ei palautusarvoja
 *
 * Tehtävät :
 * - Luo / Avaa FIFO tiedoston jonne tilaus prosessit (Order_Process) kirjoittavat
 *   HUOM! Avaus tehtävä read/write moodiin, vaikka pelkästään luetaan tiedostoa (selitys alkuteksteissä)
 * - Toimintalooppi
 *   - Lukee FIFO tiedostoon tulleen tilauksen
 *   - Tulostaa tilauksen ruudulle
 *   - Jäädään odottamaan uusia tilauksia
 *   - Poistutaan toimintaloopista kun ohjelman lopetus aktivoitu
 * - Suljetaan avoimet FD:t (File Descriptorit)
 *
 * Poistuminen/Sulkeminen :
 * - Poistutaan kun ohjelman lopetus aktivoitu 
 * - Ennen tiedostojen sulkemista, odotellaan hetki (odotus toimintoa pitää vielä kehittää)
 * - Suljetaan avoimet tiedostot ja poistetaan FIFO tiedosto
 * - Poistutaan _exit komennolla (ei aiheuteta ylimääräistä flush toimintaa muille prosesseille) 
 *
*/
void Order_Handle_Process(void) {
    
    int     fifo_fd;                // FIFO puskurin File descriptori
    int     n;                      // Luettujen merkkien määrä (ja väliaikaiskäyttö)
    char    fifo_buff[BUFF_SIZE];   // FIFO puskurista luettujen merkkien määrä
    char    dmsg_buff[BUFF_SIZE];   // Debug viestin kokoamista varten
    pid_t   oh_pid;                 // Oma prosessi id (tarvitaan debug printtauksessa)
    int     orderCounter = 0;       // Tilauslaskuri

    // Haetaan prosessitunnus (debug printtauksia varten)
    oh_pid = getpid();

    print_debug("Order_Handle_Process : Creating FIFO file", oh_pid);
    
    // Luodaan fifo tiedosto
    // Tämä prosessi käynnistyy ensin, joten täällä luodaan FIFO, jos tiedosto on olemassa (saadan virheilmoitus)
    // jatketaan eteenpäin.
    n = mkfifo(             // stat.h
            FIFO_FILE,      // Tiedosto
            0777);          // Oikeudet (kaikkille kaikki)
    
    // Jos tiedosto on jo olemassa, jatketaan eteenpäin
    if( n < 0 && errno == EEXIST) 
        print_debug("Order_Handle_Process : Creating FIFO file exist", oh_pid);
    // Joku muu virhetilanne, keskeytetään
    else if( n < 0 )
        error("ERROR creating FIFO (Order_Handle_Process)");
    
    print_debug("Order_Handle_Process : Open FIFO file for read", oh_pid);

    // Avataan FIFO tiedosto. HUOM! Pitää avata O_RDWR tilaan jotta read() funktio pysähtyy odottamaan uutta dataa.
    // Jos avaa pelkässä O_RDONLY (vain lukutila) read() funktio EI pysähdy odottamaan uutta dataa
    // vaan lukee FIFO tiedostoa loputtomassa loopissa palauttaen lukutulokseksi 0 merkkiä (kaiketi saa sieltä EOF merkin tms.)
    fifo_fd = open(         // (sys/fcntl.h)
                FIFO_FILE,  // Tiedoston nimi
                O_RDWR);    // Avauksen tila (sys/fcntl.h) : Read/Write (älä käytä O_RDONLY tilaa jos luet FIFOA jatkuvassa loopissa)

    // Tarkistus onnistuiko
    if( fifo_fd < 0 )
        error("ERROR opening FIFO for read mode (Order_Handle_Process)");

    // FIFO tiedoston lukusilmukka
    // Poistutaan kun ohjelma lopetetaan
    for(;close_program == 0;) {

        // Lisätään tilaus laskuria, lähinnä debug viestiä varten
        orderCounter++;

        // Nollataan lukupuskuri
        memset(                 // string.h
            fifo_buff,          // Nollattava muuttuja
            0,                  // Merkki millä muuttuja täytetään
            sizeof(fifo_buff)); // Paljonko merkejä laitetaan (koko muuttujaan)

        print_debug("Order_Handle_Process : Waiting FIFO file get data", oh_pid);

        // Luetaan FIFO tiedostosta puskuriin, palauttaa montako merkkiä on luettu
        // Odottaa niin kauan kuin FIFO:ssa on luettavaa, keskeytysviesti generoi virheen
        n = read(                   // unistd.h
                fifo_fd,            // FIFO puskurin File descriptori
                fifo_buff,          // Puskuri johon merkit luetaan
                sizeof(fifo_buff)); // Paljonko merkkejä yritetään lukea
        
        // Jos ohjelmaa on pyydetty sulkeutumaa
        if( close_program ) {
            print_debug("Order_Handle_Process : Terminating by signal", oh_pid);
            break;
        }
        
        // Tutkitaan virhe
        if( n < 0 )
            error("ERROR reading from FIFO (Order_Handle_Process)");
        // Tulostetaan FIFOsta saatu tieto/tilaus
        else if( n > 0 ) {

            // Ajetaan alas jos tilaus alkaa q u i t merkeillä
            if( fifo_buff[0] == 'q' && fifo_buff[1] == 'u' && fifo_buff[2] == 'i' && fifo_buff[3] == 't') {
                print_debug("Order_Handle_Process : Terminating and Send SIGQUIT group signal", oh_pid);
                killpg(getpgid(main_pid), SIGQUIT);
                break; // Poistutaan for(;;) loopista
            }
            
            // Nollataan debug viesti (memset kommentoitu ylempänä)
            memset(dmsg_buff, 0, sizeof(dmsg_buff));

            // Kasataan debug viesti jossa juokseva tilausnumero mukana
            snprintf(dmsg_buff, sizeof(dmsg_buff), "Order_Handle_Process : We got new order [%04d]", orderCounter);
            
            // Tulosetaan debug viesti
            print_debug(dmsg_buff, oh_pid);

            // Tulostetaan tilaus, tilaus tulostuu aina, olipa debug viestien tulostus päällä tahi ei
            printf("%s\n", fifo_buff);

        } // if : n > 0

    } // for : ;;

    // Koska joku ilkeämielinen taho voi kill komennolla tappaa vain tämän prosessin,
    // Lähetetään SIGQUIT ryhmäkomento muille prosesseille jotta tietävät myös tulla alas.
    killpg(getpgid(main_pid), SIGQUIT);

    // Odotellaan hetki jotta tilasten prosessoijat (Order_Process) sammuvat (ei poisteta tiedostoa mihin ne yrittää kirjoittaa)
    // Quick and dirty menetelmä, oikeasti tänne pitäisi laittaa kyttäys että kaikki avatut tilaus prosessit ovat sulkeutuneet (PID vahti)

    sleep(1);
    print_debug("Order_Handle_Process : Terminated", oh_pid);

    // Suljetaan ja poistetaan FIFO tiedosto
    close(fifo_fd);     // Suljetaan File descriptori
    unlink(FIFO_FILE);  // Poistetaan FIFO tiedosto
    _exit(0);           // Exit ilman että järjestelmä/muut prosessit flushaa puskureita
}

// Ohjelman kulun seuraamiseksi tulostellaan statustietoja
void print_debug(char *msg, pid_t pid) {
    
    if( DEBUG_MESSAGES ) {
        printf("[%05d] - %s\n", pid, msg);  // Kirjoitetaan viesti ja pid
        fflush(stdout);                     // Pakotetaan terminaali tulostamaan tekstin heti
    }
}

// Virheviestin tulostus ja terminointikomennon lähetys
// HUOM! Terminointi komento pitää lähettää ryhmäviestinä (killpg) jotta kaikki käynnissä olevat prosessit saa sen
void error(char *msg) {
 
    // Tulostetaan virhviesti (jätetään vähän tilaa ympärille että virhe-tekstin huomaa helpommin)
    printf("\n");
    perror(msg);
    printf("\n");
    // Ajetaan ohjelma alas (jos ei ole jo menossa alas)
    if( close_program == 0) {
        printf("Sending SIGQUIT signal to group\n\n");
        killpg(getpgid(main_pid), SIGQUIT);
    }
}

// Signaalin käsittelijä erilaisia lopetus signaaleja varten
static void sigHandler(int sig) { 

  // Jos ei ollut childin terminointi, määrätään ohjelma menemään alas : SIGQUIT, SIGINT ja SIGTERM viestit
  close_program = 1;

}
