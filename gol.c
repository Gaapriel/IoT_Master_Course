/*

The Game of Life 

Create a Game of Life. Game of Life is zero-player game or
a simulation of a cellular automaton. Our Game of Life will
be played in a 20x20 grid. Each gridmember will be defined
as a struct with following information:
		int alive;
		int x-coord;
		int y-coord;

First the user sets the initial conditions by changing gridmembers 
alive. After he is satisfied with the initial conditions, the game
starts. It follows the rules in each round. Rules are:

		- Each gridmember with zero or one living neighbour: dies
		- Each gridmember with two or three living neighbours: lives
		- Each gridmember with four or more living neighbours: dies
		- Each dead gridmember with three living neighbours becomes alive

The grid should span between the borders so row 20 is a neighbour of row 1 and
same with columns. Create a menu system to handle the setting of initial conditions.
During the game, display the current round (or generation). Make the game pause every
50 rounds to allow the user to either (c)ontinue or (q)uit. Quit message should display
rounds played, living gridmembers at start and at end. Store this info into a file with
date and time played.

There are about a million implementations of this program in the internet. Find the
ones that will help you to finish.

Tämän toteutuksen ajatus :

- Alustuksessa jokaiselle solulle asetetaan taulukko, jossa indeksi 0 on oma tilatieto(CELL_STAT_LIVE tai CELL_STAT_DEATH) ja naapureiden osoitteet indeksi 1-8 (samassa taulukossa on siis kahta eri tietoa)
- Kierroksella jokainen solu yhteenlaskee omien naapureiden tilatiedot (0 tai 1) ja summasta päätellään ja asetetaan oma status
- Tulostus rutiini käy läpi taulukon (laskennan jälkeen) ja näyttää solujen tilan (CELL_STAT_LIVE tai CELL_STAT_DEATH)

- Laskentakierros tehdään vasemmalta - oikealle, rivi kerrallaan
- Solut etsivät naapureita myös reunojen toiselta puolelta
- Grid alueen kokoa voi muuttaa GRID_ROWS ja GRID_COLS vakioista (terminaali ikkunan koko rajoittaa piirtämistä)

*/


#define	GRID_COLS		20								// Rivit (voi muutella vapaasti erikokoiseksi alueeksi)
#define	GRID_ROWS		20								// Sarakkeet (voi muutella vapaasti erikokoiseksi alueeksi)

#define	GRID_CELLS		GRID_COLS*GRID_ROWS				// Kaikkien solujen lukumäärä
#define GRID_SN_TOT		9 								// Tilavaraus omille ja naapureiden tiedoille
#define GRID_S_IND		0 								// Aktiivisen solun indeksi gridData:ssa
#define	RUN_TIME_S		30								// Ohjelman suoritusaika (max) sec 0 = Rajaton aika
#define	CYCLE_TIME_MS 	50								// Ohjelman sykliaika ms (ilman jarruttelua terminaali sekoilee)
#define CELL_STAT_LIVE	1								// Solu on elossa
#define CELL_STAT_DEATH	0								// Solu on kuollut
#define CALC_STAT_CHG 	1
#define CALC_STAT_UNCHG	0

/* ANSI komentoja ruudun ja osoittimen manipulointiin, nämä varmaan löytyy jostain järjestelmän .h tiedostosta -> en vielä löytänyt... */
#define A_HOME 			"\x1B[1;1H"						// Siirtää kursorin vasempaan yläreunaan
#define A_CLEAR 		"\x1b[2J"						// Tyhjentää ruudun
#define A_RESET 		"\x1b[0m"						// Palauttaa terminaalin asetukset (väri yms.)
#define A_HIDE_CURSOR 	"\x1b[?25l"						// Piilottaa kursorin
#define A_SHOW_CURSOR	"\x1b[?25h"						// Näyttää kursorin

// Kulmasolujen tyypit
enum edge_types {
	UL_CORNER	= 1,	// Ylä-Vasen kulma
	U_SIDE		= 2,	// Yläreuna
	UR_CORNER	= 3,	// Ylä-Oikea kulma
	L_SIDE		= 4,	// Vasen sivusta
	R_SIDE		= 5,	// Oikea sivusta
	DL_CORNER	= 6,	// Ala-Vasen kulma
	D_SIDE		= 7,	// Alareuna
	DR_CORNER	= 8,	// Ala-Oikea kulma
};


struct gridStruct {
		int live;		// 1 : Solun tilatieto, 2 : Sisältää yv kulmakoordinaatin 3 
		int x;			// 1 : Solun x koordinaatti, 2 : 
		int y;
};

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static unsigned short gridData[GRID_CELLS][GRID_SN_TOT];	// Varataan oikean kokoinen taulukko jokaiselle solulle ja naapureille
static int totLiveCells, roundCount;

static short data[GRID_CELLS];

int main(int argc, char *argv[]) {

long int lifeTimeMS = 0;
long int maxLifeTimeMS = 0;


char c;

	

	roundCount = 0;

	printf(A_HIDE_CURSOR A_CLEAR A_HOME);

	initData();
	userDefStart();
	//randomStart();

	maxLifeTimeMS = (long int)RUN_TIME_S * 1000;

	for (lifeTimeMS = 0; lifeTimeMS < maxLifeTimeMS; lifeTimeMS += CYCLE_TIME_MS ) {
		roundCount++;
		printData();
		usleep(CYCLE_TIME_MS * 1000);
		if (calcData() == CALC_STAT_UNCHG) {
			printf(A_SHOW_CURSOR "\nUusia soluja ei voi enää synnyttää...\n");
			return 1;
		}
		//c = getchar();
	}
	
	printf(A_SHOW_CURSOR "\nAikaraja tuli täyteen...\n");
	return 0;
}

void userDefStart() {
	int rCol, rRow;
	int dCol, dRow;

	int iResult = 1;

	printf(A_SHOW_CURSOR);
	iResult = 1;
	while (iResult != 0) {
		printData();
		printf("\n (%d %d) Anna piste [COL ROW] (0 0) : ", dCol, dRow);
		iResult = scanf("%d %d", &rCol, &rRow);
		/* Tarkistetaan että on oikealla alueella */
		if(rCol <= GRID_COLS && rCol > 0 && rRow <= GRID_ROWS && rRow > 0 ) {
			dCol = rCol - 1;
			dRow = GRID_COLS * (rRow-1);
			if(gridData[dCol+dRow][GRID_S_IND] == CELL_STAT_LIVE) {
				gridData[dCol+dRow][GRID_S_IND] = CELL_STAT_DEATH;
			} else {
				gridData[dCol+dRow][GRID_S_IND] = CELL_STAT_LIVE;
			}
		}

	}
	
}

void randomStart(void) {

int noRandomCells = 0;
int rndLoop = 0;
int rndCellID = 0;
time_t	t;
char c;
	
	srand((unsigned) time(&t));

	noRandomCells = (int)rand() % GRID_COLS / 2; // (2 * GRID_COLS);
	for(rndLoop = 0; rndLoop < noRandomCells; rndLoop++){
		rndCellID = rand() % (2 * GRID_COLS) + (GRID_CELLS / 2) - GRID_COLS;
		gridData[rndCellID][GRID_S_IND] = CELL_STAT_LIVE;
	}
}

void printData(void) {
int printLoop = 0;
int oldLineNo = 0;
int lineNo = 0;
char colLine;
div_t prc;

	
	oldLineNo = 0;
	printf(A_CLEAR A_HOME "Grid size : %03d  [%03d/%03d]\n┌",
		GRID_CELLS,GRID_ROWS,GRID_COLS);
	
	for (printLoop = 0; printLoop < GRID_COLS; printLoop++ ) {
		printf("─");
	}
	printf("┐\n│");

	for( printLoop = 0; printLoop < GRID_CELLS; printLoop++) {

		/* Haetaan rivi ja sarakenumero */
		prc = div(printLoop, GRID_COLS);
		lineNo = prc.quot;

		if (oldLineNo!=lineNo) {
			printf("│\n│");
			oldLineNo = lineNo;
		}

		if(gridData[printLoop][GRID_S_IND] == CELL_STAT_LIVE) {
			printf("#");
		} else
		{
			printf(" ");
		}
	}
	printf("│\n└");
	for (printLoop = 0; printLoop < GRID_COLS; printLoop++ ) {
		printf("─");
	}

	if (totLiveCells!=0) {
		printLoop = totLiveCells * 100 / GRID_CELLS;
	} else {
		printLoop = 0;
	}
	
	printf("┘\nLive Cells : %04d (%03d) Death Cells : %04d  Round : %04d     ",
		totLiveCells,printLoop, GRID_CELLS-totLiveCells,roundCount);
}

int calcData(void) {

int calcLoop = 0;
int calcGridIdx = 0;
int calcSum = 0;
int statusChanged = 0;

	calcSum = 0;
	statusChanged = CALC_STAT_UNCHG;
	totLiveCells = 0;

	/* Käydään jokainen solu läpi */
	for( calcLoop = 0; calcLoop < GRID_CELLS; calcLoop++) {
		/* Summataan k.o solun naapurien status */
		calcSum =
			gridData[gridData[calcLoop][UL_CORNER]][GRID_S_IND] +
			gridData[gridData[calcLoop][U_SIDE]][GRID_S_IND] +
			gridData[gridData[calcLoop][UR_CORNER]][GRID_S_IND] +
			gridData[gridData[calcLoop][L_SIDE]][GRID_S_IND] +
			gridData[gridData[calcLoop][R_SIDE]][GRID_S_IND] +
			gridData[gridData[calcLoop][DL_CORNER]][GRID_S_IND] +
			gridData[gridData[calcLoop][D_SIDE]][GRID_S_IND] +
			gridData[gridData[calcLoop][DR_CORNER]][GRID_S_IND];

		/* Katsotaan miten k.o solun käy. Säännöt :
		- Each gridmember with zero or one living neighbour: dies
		- Each gridmember with two or three living neighbours: lives
		- Each gridmember with four or more living neighbours: dies
		- Each dead gridmember with three living neighbours becomes alive
		*/
		switch(calcSum) {
			case 2 :
				break;
			case 3 :
				if (gridData[calcLoop][GRID_S_IND] == CELL_STAT_DEATH) {
					statusChanged = CALC_STAT_CHG;
					gridData[calcLoop][GRID_S_IND] = CELL_STAT_LIVE;
				}
				break;

			default :
				/* Tapetaan */
				if (gridData[calcLoop][GRID_S_IND] == CELL_STAT_LIVE) {
					statusChanged = CALC_STAT_CHG;
					gridData[calcLoop][GRID_S_IND] = CELL_STAT_DEATH;
				}
		}
		totLiveCells += gridData[calcLoop][GRID_S_IND];
	}
	return statusChanged;
}

void initData(void) {

#define GRID_DATA_INIT	1				// gridData solun alustus aloitettu
#define GRID_DATA_OK 	0				// gridData solun alustus valmis
#define GRID_FIRST_ROW	0				// Ensimäisen rivin nro
#define GRID_LAST_ROW	GRID_ROWS - 1	// Viimeisen rivin nro
#define GRID_FIRST_COL	0				// Ensimäisen sarakkeen nro
#define GRID_LAST_COL	GRID_COLS - 1	// Viimeisen sarakkeen nro	

int col_number;	// Aktiivinen sarakenumero
int row_number;	// Aktiivinen rivinumero
int countLives = 0; // Elossa olevien naapurien lukumäärä

	for( row_number = 0; row_number < GRID_ROWS; row_number++) {
		/* Tutkitaan rivit */
		switch(row_number) {
			case GRID_FIRST_ROW : // Ensimäinen rivi
				// Tarkistetaan sarakkeet
				for(col_number = 0; col_number < GRID_COLS; col_number++){
					switch(col_number) {
						case GRID_FIRST_COL : // Vasen yläkulma	
							countLives =+ data[GRID_CELLS - 1];	// Vasen kulma, yläpuoli
							countLives =+ data[GRID_CELLS - GRID_COLS];			// Yläpuolella
							countLives =+ data[GRID_CELLS - GRID_COLS] + 1;		// Oikea kulma, yläpuoli
							countLives =+ data[GRID_COLS - 1];					// Vasemmalla
							countLives =+ data[1];								// Oikealla
							countLives =+ data[2 * GRID_COLS - 1];				// Vasen kulma, alapuoli
							countLives =+ data[GRID_COLS];						// Suoraan alapuolella
							countLives =+ data[GRID_COLS + 1];					// Oikea kulma, alapuolella
							break;
						case GRID_LAST_COL : // Oikea yläkulma
							countLives =+ data[GRID_CELLS - 2];		 			// Vasen kulma, yläpuoli
							countLives =+ data[GRID_CELLS - 1];					// Yläpuolella
							countLives =+ data[GRID_CELLS - GRID_COLS];			// Oikea kulma, yläpuoli
							countLives =+ data[GRID_COLS - 2];					// Vasemmalla
							countLives =+ data[0];								// Oikealla
							countLives =+ data[2 * GRID_COLS - 2];				// Vasen kulma, alapuoli
							countLives =+ data[2 * GRID_COLS - 1];				// Suoraan alapuolella
							countLives =+ data[GRID_COLS];						// Oikea kulma, alapuolella
							break;
						default : // Yläreunan muut solut
							// Yläreunan solut
							gridData[mcl][UL_CORNER]	= mcl + (GRID_ROWS - 1) * GRID_COLS - 1;	// Vasen kulma, yläpuoli
							gridData[mcl][U_SIDE]		= mcl + (GRID_ROWS - 1) * GRID_COLS;		// Yläpuolella
							gridData[mcl][UR_CORNER]	= mcl + (GRID_ROWS - 1) * GRID_COLS + 1;	// Oikea kulma, yläpuoli
							gridData[mcl][L_SIDE]		= mcl - 1;									// Vasemmalla
							gridData[mcl][R_SIDE]		= mcl + 1;									// Oikealla
							gridData[mcl][DL_CORNER]	= mcl + GRID_COLS - 1;						// Vasen kulma, alapuoli
							gridData[mcl][D_SIDE]		= mcl + GRID_COLS;							// Suoraan alapuolella
							gridData[mcl][DR_CORNER]	= mcl + GRID_COLS + 1;						// Oikea kulma, alapuolella
							gridData[mcl][GRID_S_IND]	= GRID_DATA_OK;
							break;
					} // switch(col_number)
				}
				break;

			case GRID_LAST_ROW : // Viimeinen rivi
				// Tarkistetaan sarakkeet
				switch(col_number) { // Tutkitaan mikä rivi
					case GRID_FIRST_COL : // Vasen alakulma
						gridData[mcl][UL_CORNER]	= GRID_CELLS - GRID_COLS - 1;		// Vasen kulma, yläpuoli
						gridData[mcl][U_SIDE]		= GRID_CELLS - 2 * GRID_COLS;		// Yläpuolella
						gridData[mcl][UR_CORNER]	= GRID_CELLS - 2 * GRID_COLS + 1;	// Oikea kulma, yläpuoli
						gridData[mcl][L_SIDE]		= GRID_CELLS - 1;					// Vasemmalla
						gridData[mcl][R_SIDE]		= GRID_CELLS - GRID_COLS + 1;		// Oikealla
						gridData[mcl][DL_CORNER]	= GRID_COLS - 1;					// Vasen kulma, alapuoli
						gridData[mcl][D_SIDE]		= 0;								// Suoraan alapuolella
						gridData[mcl][DR_CORNER]	= 1;								// Oikea kulma, alapuolella
						gridData[mcl][GRID_S_IND]	= GRID_DATA_OK;
						break;
					case GRID_LAST_COL : // Oikea alakulma
						gridData[mcl][UL_CORNER]	= GRID_CELLS - GRID_COLS - 2;		// Vasen kulma, yläpuoli
						gridData[mcl][U_SIDE]		= GRID_CELLS - GRID_COLS - 1;		// Yläpuolella
						gridData[mcl][UR_CORNER]	= GRID_CELLS - 2 * GRID_COLS;		// Oikea kulma, yläpuoli
						gridData[mcl][L_SIDE]		= GRID_CELLS - 2;					// Vasemmalla
						gridData[mcl][R_SIDE]		= GRID_CELLS - GRID_COLS;			// Oikealla
						gridData[mcl][DL_CORNER]	= GRID_COLS - 2;					// Vasen kulma, alapuoli
						gridData[mcl][D_SIDE]		= GRID_COLS - 1;					// Suoraan alapuolella
						gridData[mcl][DR_CORNER]	= 0;								// Oikea kulma, alapuolella
						gridData[mcl][GRID_S_IND]	= GRID_DATA_OK;
						break;
					default : // Alareunan solut
						gridData[mcl][UL_CORNER]	= mcl - GRID_COLS - 1;						// Vasen kulma, yläpuoli
						gridData[mcl][U_SIDE]		= mcl - GRID_COLS;							// Yläpuolella
						gridData[mcl][UR_CORNER]	= mcl - GRID_COLS + 1;						// Oikea kulma, yläpuoli
						gridData[mcl][L_SIDE]		= mcl - 1;									// Vasemmalla
						gridData[mcl][R_SIDE]		= mcl + 1;									// Oikealla
						gridData[mcl][DL_CORNER]	= mcl - (GRID_ROWS - 1) * GRID_COLS - 1;	// Vasen kulma, alapuoli
						gridData[mcl][D_SIDE]		= mcl - (GRID_ROWS - 1) * GRID_COLS;		// Suoraan alapuolella
						gridData[mcl][DR_CORNER]	= mcl - (GRID_ROWS - 1) * GRID_COLS + 1;	// Oikea kulma, alapuolella
						gridData[mcl][GRID_S_IND]	= GRID_DATA_OK;
						break;
				} // switch(col_number)
				break; // GRID_ROWS - 1

			default : // Ei ylä- tai alarivi
				// Tarkistetaan sarakkeet
				switch(col_number) { // Tutkitaan mikä rivi
					case GRID_FIRST_COL : // Vasen reuna
						gridData[mcl][UL_CORNER]	= mcl - 1;					// Vasen kulma, yläpuoli
						gridData[mcl][U_SIDE]		= mcl - GRID_COLS;			// Yläpuolella
						gridData[mcl][UR_CORNER]	= mcl - GRID_COLS + 1;		// Oikea kulma, yläpuoli
						gridData[mcl][L_SIDE]		= mcl + GRID_COLS - 1;		// Vasemmalla
						gridData[mcl][R_SIDE]		= mcl + 1;					// Oikealla
						gridData[mcl][DL_CORNER]	= mcl + 2 * GRID_COLS - 1;	// Vasen kulma, alapuoli
						gridData[mcl][D_SIDE]		= mcl + GRID_COLS;			// Suoraan alapuolella
						gridData[mcl][DR_CORNER]	= mcl + GRID_COLS + 1;		// Oikea kulma, alapuolella
						gridData[mcl][GRID_S_IND]	= GRID_DATA_OK;
						break;
					case GRID_LAST_COL : // Oikea reuna
						gridData[mcl][UL_CORNER]	= mcl - GRID_COLS - 1;		// Vasen kulma, yläpuoli
						gridData[mcl][U_SIDE]		= mcl - GRID_COLS;			// Yläpuolella
						gridData[mcl][UR_CORNER]	= mcl - 2* GRID_COLS + 1;	// Oikea kulma, yläpuoli
						gridData[mcl][L_SIDE]		= mcl - 1;					// Vasemmalla
						gridData[mcl][R_SIDE]		= mcl - GRID_COLS + 1;		// Oikealla
						gridData[mcl][DL_CORNER]	= mcl + GRID_COLS - 1;		// Vasen kulma, alapuoli
						gridData[mcl][D_SIDE]		= mcl + GRID_COLS;			// Suoraan alapuolella
						gridData[mcl][DR_CORNER]	= mcl + 1;					// Oikea kulma, alapuolella
						gridData[mcl][GRID_S_IND]	= GRID_DATA_OK;
						break;
					default : // Ei reunasolu
						gridData[mcl][UL_CORNER]	= mcl - GRID_COLS - 1;	// Vasen kulma, yläpuoli
						gridData[mcl][U_SIDE]		= mcl - GRID_COLS;		// Yläpuolella
						gridData[mcl][UR_CORNER]	= mcl - GRID_COLS + 1;	// Oikea kulma, yläpuoli
						gridData[mcl][L_SIDE]		= mcl - 1;				// Vasemmalla
						gridData[mcl][R_SIDE]		= mcl + 1;				// Oikealla
						gridData[mcl][DL_CORNER]	= mcl + GRID_COLS - 1;	// Vasen kulma, alapuoli
						gridData[mcl][D_SIDE]		= mcl + GRID_COLS;		// Suoraan alapuolella
						gridData[mcl][DR_CORNER]	= mcl + GRID_COLS + 1;	// Oikea kulma, alapuolella
						gridData[mcl][GRID_S_IND]	= GRID_DATA_OK;
						break;
				} // switch(col_number)
				break;
		} // Switch row_number
	} // for loop : row_number 
}