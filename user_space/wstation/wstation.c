/*
 *   Opettaja : Erno Hentunen
 *
 *   Tehtävä :
 *
 *   Virtuaalinen sääasema
 *   • Luodaan virtuaalinen sääasema.
 *   • Ensin luodaan main, joka forkkaa kolme lasta. Nämä edustavat etäsensoreita. Luodaan lämpötila, tuuli ja sademäärämittarit.
 *   • Sensorit toimivat siten, että ne luovat tasaisin väliajoin, esim 500 millisekunnin välein, ”dataa” 
 *     satunnaislukugeneraattorilla. Pitäkää ne lukuvälit järkevinä. Ei mitään Jupiterin tuulia tai Auringon pinnan lämpötiloja
 *   • Jokaisen mittauksen jälkeen sensori lähettää mittatuloksen paikallisen socketin läpi. Tätä toistetaan ikiluupissa.
 *   • Kun pääohjelma on luonut sensorit forkilla se luo kolme säiettä, jotka edustavat mittaustuloksen kerääjiä.
 *   • Nämä istuvat paikallisen socketin toisessa päässä ja keräävät datan talteen. Joka viides sekunti ne luovat keskiarvon jo 
 *     mitatuista datoista ja tallentavat sen globaaliin muuttujaan.
 *   • Pääohjelma odottaa wait_conditionin ympärillä kuten esimerkissä threads3.c. Eli kun jokainen mittasäe on saanut
 *     keskiarvon luotua omasta datastaan ne lisäävät +1 johonkin globaaliin arvoon. Kun sen arvo on 3 niin viimeinen 
 *     säie herättää mainin, joka printtaa tulokset, muuttaa arvon takaisin nollaan ja menee nukkumaan uudelleen nukkumaan
 *     wait_conditionin ympärille.
 *    
 *    Virtuaalinen sääasema
 *   • Bonus: lisää datan tallennus levylle sekä yksittäisille mittatuloksille että keskiarvoille. 
 *     Nämä voivat olla eri tiedostoissa.
 *
 * ------------------------------------------------------------------------------------------------------------------  
 * 
 *   wstation.c
 *
 *   Harjoitustyön tekijä : Erno Kilpeläinen (erno.kilpelainen@hotmail.com)
 *
 *   HUOM! 
 *
 *   - Käännä : -lpthread parametrilla
 *   - Ohjelmaa muutettu alkuperäisestä määrityksestä siten, että NUMBER_OF_SENSORS 
 *     vakiota muuttamalla voi generoida halutun määrän virtuaalisia sensoreita.
 *     Jos sensoreita on enemmän kuin sensori tyyppejä, arvotaan loppujen sensorien tyyppi randomilla
 *     (voi olla esim. useampi lämpötila-anturi yms.)
 *
 *   Käytetyt tekniikat :
 *   - Socket kommunikointi
 *   - Prosessien luonti (fork)
 *   - Säikeistys
 *   - Jaettujen resurssien lukitukset (mutex, conditional mutex)
 *   - Prosessien- ja säikeiden välinen signalointi
 *
 *   Ohjelman käynnistys :
 *   - Käynnistyy ilman parametreja
 *
 *   Tiedossa olevia puutteita :
 *   - Ohjelmasta puuttuu socket datan tarkistukset (menikö data perille, oliko data oikeassa formaatissa jne)
 *   - Käynnistys prarametreja ei ole, vaan asetukset on vakioissa.
 *   - Ohjelman kulun kommentointi on hieman niukahko ja vahingossa tuli kirjoitettua Englanniksi
 *   - Jos keksii uusia virtuaalisia sensorityyppejä (SENSOR_TYPES), uusen sensorin
 *     arvo-alueet pitää lisätä käsin sensor-funktion sensor_setup[][] tauluun.
 *     Muista lisätä uudet sensorit myös mainin lähettämään debug viestiin.
 *
 *   Ohjelman toiminnasta :
 *   - Sensori prosessille annetaan sensori tyyppi fork-vaiheessa (tietää generoida oikean tyyppistä dataa)
 *   - Sensorien luku-thread ei ole kiinnostunut minkä tyyppistä sensori dataa se vastaanottaa :
 *     - Vastaanotettu sensorin data-tyyppi löytyy vastaanotetusta datasta
 *     - Therad laskee ja sijoittaa datan vastaavaan data-tyyppi kohtaan, globaaleihin muuttujiin
 *       (glob_avg[] ja glob_avc[]) joissa on soluja yhtä paljon kuin sensori tyyppejä.
 *   - Datan formaati on : <type>:<data> (esim. 0:-10 = Lämpötila dataa, -10 astetta celsiusta)
 *   - Keskiarvon laskentaan on kaksi tapaa :
 *     - AVERAGE_MODE = 0 : Keskiarvo kaikesta kertyneestä datasta
 *     - AVERAGE_MODE = 1 : Keskiarvo täyden pthread_cond_t- syklin jäkeen, historia nollataan
 *
 *   Ohjelman sammuttaminen :
 *   - ctrl c
 *   - Poistamalla joku ohjelman prosessi kill komennolla
 *
 *   En tähän hätään kerinnyt tehdä perusteellisemmin.
 *   Palautetta voi antaa säköpostiin tai tulemalla juttusille.
 *   Älkää välittäkö kirjoitusvirheistä!
 *
 *   Erno Kilpeläinen (erno.kilpelainen@hotmail.com)
 *
 */

#define DEBUG_MESSAGES      1
#define SERVER_ADDRESS      "127.0.0.1"   // Server address
#define SERVER_PORT         5000          // Default port for server listen and clients to conncet
#define BUFF_SIZE           100           // Buffer size for transmit information thru Socket
#define SENSOR_TYPES        3             // How many different sensor types we got if add sensors, add values in sensor_setup[][] array
#define NUMBER_OF_SENSORS   3             // Number of sensors (sensor tasks)
#define AVERAGE_MODE        1             // 0 = All time average, 1 = Current cycle average

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>

pthread_mutex_t sensor_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  sensor_cond  = PTHREAD_COND_INITIALIZER;

int glob_avg[SENSOR_TYPES]; // Storage for sensors average data
int glob_avc[SENSOR_TYPES]; // Number of each avg cell where average value is calculated
int glob_count;             // Count for sensor readers who has write data

int keep_running;           // Program close signal activation
pid_t main_pid;             // Main program pid for shut down group message

// Prototypes
void sensor(int type);                  // Sensor data sender module (task)
void *sensor_reader(void *arg);         // Sensor data reader module (main process thread)
void error(char *msg);                  // Error handler
void print_debug(char *msg, pid_t pid); // Debug printing 
static void sigHandler(int sig);        // Program shut down signal handler


/* Main
 *
 * Startup :
 *  - Program start here 
 *
 * Startup parameters :
 *  - No parameters
 *
 * Tasks :
 *  - Do init for variables
 *  - Init signals for program shut down
 *  - Fork up loop : Fork all sensors up (first 3 sensors has fixed type, rest is random from sensor types)
 *  - Get in socket server listen loop :
 *    - Accept sensor Socket connection
 *    - Check, if client is local (same as server address)
 *    - Create new thread for sensor with Socket FD parameter
 *    - Exit listen loop after all sensors has connected
 *  - Close un-used file descriptor
 *  - Infor user about average mode (how long time data is collected, 0 = Whole history, 1 = Last full cycle)
 *  - Get in to mutex loop (stay as long ctrl c or kill command) :
 *    - Wait pthread_cond_wait from threads
 *    - Get global data and form print message
 *    - Reset data if need (AVERAGE_MODE=1)
 *    - Release mutex
 *    - Print out pre-formed message
 *  - Free reserved resources
 *  - Send quit command to others
 *  - End program
 *
 */
int main(int argc, char *argv[]) {
    
    #define SERV_MAX_WCONS      5   // Max waiting socket connections
    
    struct sockaddr_in serv_addr;   // Socket, server address settings
    struct sockaddr_in cli_addr;    // Socket, client address settings
    struct sigaction sa;            // For termination signals
    pid_t child_pid;                // Forking
    time_t t;                       // For random number initialize 
    uint clilen;                    // For socket client settings
    pthread_t thread;               // For thread creating
    int i, i2;                      // Loops
    int sockfd, newsockfd;          // File descriptors for Socket (server and accept)
    int *newsock = NULL;            // Pointer for thread parameter
    char buff[BUFF_SIZE];           // Debug message buffer (print out average values)
    char *cli_ip;

    // Silence compiler warning about un-used variable
    argc = 0;
    argv = NULL;

    // Get pid for debug purposes
    main_pid = getpid();

    // Init conditional wait
    pthread_cond_init(&sensor_cond, NULL);

    // Init global data
    for(i = 0; i < SENSOR_TYPES; i++) {
        glob_avg[i] = 0; // Average calculated values of all measurement
        glob_avc[i] = 0; // Number of values what form average values
    }
    
    glob_count = 0;

    // Init signals for program terminating
    keep_running = 1;
    sa.sa_handler = sigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if( sigaction(SIGQUIT, &sa, NULL) == -1 )
        error("Problem installing signal handler SIGQUIT (main)");

    if( sigaction(SIGINT, &sa, NULL) == -1 )
        error("Problem installing signal handler SIGINT (main)");

    if( sigaction(SIGTERM, &sa, NULL) == -1 )
        error("Problem installing signal handler SIGTERM (main)");

    // Init random number generator
    srand((unsigned) time(&t));

    // Reserve memory for thread parameter ptr
    newsock = malloc(sizeof(int));
    // Checking (even we reserve just one int)
    if( newsock == NULL )
        error("ERROR Cant malloc memory");
    
    // Print debug info
    print_debug("Main   : Forking sensors", main_pid);
 
    // Forking up all sensors (sensor tasks)
    for(i = 0; i < NUMBER_OF_SENSORS; i++) {
        
        child_pid = fork();
        if( child_pid == 0 ) {
            // Selecting type of sensor to create, at least one of each
            // sensor type must exist, unless we got less sensors than sensor types.
            if( i < SENSOR_TYPES )
                i2 = i;
            else
                i2 = rand() % 3;
            sensor(i2);
            return 0;
        } 
        else if( child_pid == -1 )
            error("ERROR forking sensor (main)");
        else
            // Sleep 200ms
            usleep(200 * 1000);
    }

    // Plot some debug info
    print_debug("Main   : Making socket handler", main_pid);

    // Setting up socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    // Error check
    if( sockfd < 0 ) 
        error("ERROR opening socket (main)");

    // Reseting server address struct
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    // Server settings to struct
    serv_addr.sin_family        = AF_INET;
    serv_addr.sin_addr.s_addr   = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port          = htons(SERVER_PORT);

    // Set socket parameters (disable "address allready used" error)
    i = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int));

    // Bind address settings to socket fd
    if( bind( sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding (main)");

    // Start listening socket
    if( listen( sockfd, SERV_MAX_WCONS) < 0 )
        error("ERROR on listen (main)");
    
    // Size definiton for clien address struct
    clilen = sizeof(cli_addr);

    i = 0;

    print_debug("Main   : Waiting connections", main_pid);

    // Loop for catch sensor connections
    for(;keep_running;) {

        // Accept new socket connection
        newsockfd = accept( sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if( newsockfd < 0 ) {
            error("Error accept connection");
            break;
        }

        // Pick client IP address
        cli_ip = inet_ntoa(cli_addr.sin_addr);

        // Accept connection if its coming from same computer (localhost/127.0.0.1)
        if( strcmp(cli_ip, SERVER_ADDRESS) == 0 ) {

            // Thread parameter
            *newsock = newsockfd;

            // Popping up thread for read sensor data        
            if( pthread_create(&thread, NULL, sensor_reader, (void*) newsock) < 0 ) {
                error("Cant create thread");
                break;
            }
            
            // Mark thread as deatch (no need to join for free thread resources)
            pthread_detach(thread);
            
            i++;

            // If all sensors has been reported, continue
            if( i >= NUMBER_OF_SENSORS )
                break;
        } 
        else
            close(newsockfd);

    } // while : 

    // Free server resources
    close(sockfd);

    if(AVERAGE_MODE)
        print_debug("Main   : Start print averages (averages in cycle mode)", main_pid);
    else
        print_debug("Main   : Start print averages (average in total history mode)", main_pid);

    // Mutex loop
    for(;keep_running;) {

        sleep(2);

        // Clear degug message buffer
        memset(buff, 0, BUFF_SIZE);

        // Waiting for thread kick
        pthread_cond_wait(&sensor_cond, &sensor_mutex);

        // Reseting global average write counter
        glob_count = 0;

        // Build up debug message
        snprintf(buff, BUFF_SIZE, "[Temp/Wind/Rain] = [%03d/%02d/%02d]", glob_avg[0], glob_avg[1], glob_avg[2]);

        // Check average calculation mode, 1 = Cycle mode > Clear buffers
        if( AVERAGE_MODE )
            for(i = 0; i < SENSOR_TYPES; i++) {
                glob_avg[i] = 0; // Average calculated values of all measurement
                glob_avc[i] = 0; // Number of values what form average values
            }

        // Release mutex 
        pthread_mutex_unlock(&sensor_mutex);

        // We can print debug message after release mutex 
        print_debug(buff, main_pid);

    }

    // Release mutex and conditional wait
    pthread_cond_destroy(&sensor_cond);
    pthread_mutex_destroy(&sensor_mutex);

    free(newsock);
    close(newsockfd);

    // Send quit command to others
    killpg(getpgid(main_pid), SIGQUIT);

    // Wait small time for threads give debug message about exit
    sleep(3);

    print_debug("Main   : Exit", main_pid);

    exit(0);

}

/* Sensor program (separated tasks)
 *
 * Startup :
 *  - main task create before go wait sockets 
 *
 * Startup parameters :
 *  - Sensor type
 *
 * Tasks :
 *  - Do init for variables
 *  - Make setups for Socket client
 *  - Try connect to server with retry loop
 *  - Get in action loop :
 *    - Generate random weather data for current sensor type (get in parameter)
 *    - Write weather data to Socket
 *    - Sleep until next generation
 *  - Exit action loop if program is shut down (keep_running)
 *  - Release used resources
 *  - Send quit command to others
 *  - _exit
 */
void sensor(int type) {

    #define DATA_SEND_DELAY_MS  500 // Data send delay in milliseconds
    #define CONN_MAX_RETRY      10  // Max connection retry count
    #define CONN_RETRY_DELAY_MS 1500 // Delay between connection retryes

    // Data range values for sensor type (using with rand() function)
    int sensor_setup[3][2] =  { {100, -40 },    // Temperature sensor celsius
                                { 10,   0 },    // Wind sensor meters/second
                                {  4,   0 }};   // Rain sensor mm/day

    int sockfd;                     // File descriptor for socket
    struct sockaddr_in serv_addr;   // Server socket address struct
    pid_t pid;                      // Pid number for debug messages
    char buff[BUFF_SIZE];           // Revice buffer (get feedback from server)
    int act_loop_exit;              // Main action loop exit tricker, 0 = Stay inside loop, !0 = Exit
    int act_loop_counter;           // Counter for loop (now used in connection retry loop only)

    // Get own pid for debug purposes
    pid = getpid();
    
    // Print out debug information
    print_debug("Sensor : Connecting server", pid);

    // Init Socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if(sockfd == -1) {
        error("Cant create socket");
        _exit(1);
    }

    // Reset server address struct
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    // Add server settings for connection
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(SERVER_PORT);
    
    // Init variables before action loop
    act_loop_exit = act_loop_counter = 0;

    // Testing for loop connection
    while( !act_loop_exit ) {
        // Try connect socket
        if( connect(sockfd , (struct sockaddr *)&serv_addr , sizeof(serv_addr)) == -1) {
            
            // Counting retry times
            act_loop_counter++;

            // Exit if try too many times or program is getting down
            if( act_loop_counter > CONN_MAX_RETRY || !keep_running ) {
                act_loop_exit = -1;
                break;
            }
            // Wait until next connection
            usleep(CONN_RETRY_DELAY_MS * 1000);
            
            // Re-Create socket after failed connection
            sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if(sockfd == -1) {
                error("Cant create socket");
                _exit(1);
            }
        }
        else
        {
            act_loop_exit = 0;
            break;
        }  
    }

    // Print out debug information
    print_debug("Sensor : Start generating weather data", pid);

    // Action loop, until exit
    for(;keep_running;) {

        // Clear data buffers
        memset(buff, 0, BUFF_SIZE);

        // Form message to send, <data type>:<data>
        snprintf(buff, BUFF_SIZE, "%d:%d", type, rand() % sensor_setup[type][0] + sensor_setup[type][1]);

        // Write buffer to socket 
        if( write(sockfd, buff, sizeof(buff)) < 0 ) {
            error("Sensor : Cant write data to socket");
            break;
        }

        // Get to sleep before new actions
        usleep(DATA_SEND_DELAY_MS * 1000);

    } // for : action_loop_exit

    // Close conncetion
    close(sockfd);
    
    // Send quit command to others
    killpg(getpgid(main_pid), SIGQUIT);

    print_debug("Sensor : Exit", pid);

    _exit(0);
}

/* Sensor readers (threads)
 *
 * Startup :
 *  - main task create after sensor task has connected to server 
 *
 * Startup parameters :
 *  - Socket file descriptor
 *
 * Tasks :
 *  - Do init for variables
 *  - Get in action loop :
 *    - Read data from Socket file descriptor (parameter)
 *    - Collect data for average calculation
 *    - If get enough data :
 *      - Lock global storage variable with mutex
 *      - Add existing global variable (glob_avg[]), current calculated average value
 *      - Add write count to global variable (glob_count)
 *      - Check if all sensors has write data to glob_avg[] with glob_count
 *      - If all has write, kick main task with pthread_cond_broadcast
 *  - Exit action loop if program is shut down (keep_running)
 *  - Release used resources
 *
 */
void *sensor_reader(void *arg) {

    #define AVG_NUMBERS 10      // How many read cycles goes before make average calculation

    int tfd;                    // File descriptor for socket
    char buff[BUFF_SIZE];       // Revice buffer (get feedback from server)
    int n1;                     // How much data is readed to buffer from socket
    int avg_sum, i;             // For calculating averages from data
    int data_value, data_type;  // Input data and data type variables (read from buffer)

    // Get file descriptor from arguments
    tfd = *(int*)arg;

    // Sending status info
    print_debug("Thread : Start read and process weather data", tfd);

    // Reset variables before loop
    i = avg_sum = 0;

    // Action loop
    for(;keep_running;) {

        // Reset buffer
        memset(buff, 0, BUFF_SIZE);

        // Reading data from socket file descriptor
        n1 = read(tfd, buff, BUFF_SIZE);

        // Check what happend with read
        if(n1 < 0) {
            error("Error reading data");
            break;
        } 

        // No data, sleep some, before try again (sleep in ms format)
        else if( n1 == 0 ) {
            usleep(500 * 1000);
        }

        // We got some data
        else {

            // Format buffer data to variables 
            sscanf(buff,"%d:%d", &data_type, &data_value);
            
            // Add data to sum
            avg_sum += data_value;

            // Add counter value for average check
            i++;

            // If time to calculate average and write it to global variable
            if( i > AVG_NUMBERS ) {

                // Lock global variable
                pthread_mutex_lock(&sensor_mutex);

                // Add average value (make history values and current values equeal in calculation)
                glob_avg[data_type] =
                    (glob_avg[data_type] * glob_avc[data_type] + avg_sum) / 
                    (glob_avc[data_type] + AVG_NUMBERS);
            
                // Add existing numbers what form average values
                glob_avc[data_type] += AVG_NUMBERS;
                
                // Add global counter
                glob_count++;
                
                // If our global counter is "full", wake up main to clean it up
                if( glob_count >= NUMBER_OF_SENSORS )
                    pthread_cond_broadcast(&sensor_cond);

                // Release lock
                pthread_mutex_unlock(&sensor_mutex);
                
                i = avg_sum = 0;
            }

            // If you want see what data is moving inside socket, 
            // un-comment line below compile & run (print out lots of data)

            //printf("[Thread : %d] glob_count : %d  Data [type:data] : %s\n", tfd, glob_count, buff);
        }
    }

    // Release main condition lock
    pthread_cond_broadcast(&sensor_cond);

    // Release file descriptor
    close(tfd);
    print_debug("Thread : Exit", tfd);
    pthread_exit(NULL);
}

// Error handler with sending quit signal
void error(char *msg) {
 
    // Make space around error message (more easy to noticed it)
    if( keep_running ) {
        printf("\n");
        perror(msg);
        printf("\n");
        printf("Sending SIGQUIT signal to group\n\n");
        killpg(getpgid(main_pid), SIGQUIT);
    }
}

// Printing out some status info with message and sender information (task id or thread id)
void print_debug(char *msg, pid_t pid) {
    
    if( DEBUG_MESSAGES ) {
        // Print out debug message
        printf("[%05d] - %s\n", pid, msg);
        // Force stdout write data to screen
        fflush(stdout);
    }
}

// This function is alarmed when catch signals whst has marked as followe
static void sigHandler(int sig) { 

    // Termination signal arrived, ask program shut down
    keep_running = 0;

    // Silence compiler warning about un-used variable (works in clang or gcc in osx)
    sig = 0;

}