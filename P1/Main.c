#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>

typedef struct DP_cell{
	int s; int i; int d;
} DP_cell;


DP_cell **Matrix; // stores the dynamically computed matrix
int SW = 0; // Smith-Waterman or Needleman-Wunsch
int ma, mi, h, g; // scoring parameters
int maxRowIndex = 0 , maxColIndex = 0; // stores index of max value to back track from
char *s1; // stores sequence 1
char *s2; // stores sequence 2
char *gene1; // stores name of sequence 1
char *gene2; // stores name of sequence 2

// other helper variables
char *charPtr;  
int countStr = 0;

// helper function to return max of 3 integers
int max(int a, int b, int c){
	return (a > b ? (a > c ? a : c) : (b > c ? b : c));
}

// helper function to reverse a string
char* stringReverse(char *str){

	char *start = str;
	char *end = start+strlen(str)-1;

	while(start<end)
	{
		char temp = *end;
		*end = *start;
		*start=temp;
		start++;
		end--;
	}
	return str;
}

// choose the best between substitution, deletion and insertion
int chooseDirection(int i, int j){
	return max(Matrix[i][j].s, Matrix[i][j].d, Matrix[i][j].i);
}

// calculate the substitution score
int substitution(char *s1, char *s2, int i, int j){
	if (s1[i - 1] != s2[j - 1]) 
		return chooseDirection(i-1,j-1) + mi;
	else
		return chooseDirection(i-1,j-1) + ma;	
}

// dynamically allocate the memory for DP_cell matrix
int allocate(int m, int n){
	
	int i;	
	DP_cell **mat = (DP_cell **)malloc(m * sizeof(DP_cell *));
	if (mat == 0){
		printf("Error allocating memory");
		return -1;
	}
	for (i = 0; i < m; i++){
		mat[i] = (DP_cell *)malloc(n * sizeof(DP_cell));
		if(mat[i] == NULL){
			printf("Error allocating memory");
			return -1;
		}
	}
	Matrix = mat;
	return 0;
}

// initialise row 0 and col 0 of matrix
void initialiseMatrix(int m, int n){

	int i, j;

	Matrix[0][0].s = 0;
	Matrix[0][0].d = 0;
	Matrix[0][0].i = 0;
	
	for (i = 1; i <= m; i++){
		if (!SW){
			// global alignment
			Matrix[i][0].s = INT_MIN - (h + (2 * ((m > n) ? m : n) * g));
			Matrix[i][0].d = h + (g * (i));
			Matrix[i][0].i = INT_MIN - (h + (2 * ((m > n) ? m : n) * g));
		}
		else {
			// local alignment
			Matrix[i][0].s = 0;
			Matrix[i][0].d = 0;
			Matrix[i][0].i = 0;
		}
	}
	for (j = 1; j <= n; j++){
		if (!SW){
			// global alignment
			Matrix[0][j].s = INT_MIN - (h + (2 * ((m > n) ? m : n) * g));
			Matrix[0][j].d = INT_MIN - (h + (2 * ((m > n) ? m : n) * g));
			Matrix[0][j].i = h + (g * (j));
		}
		else {
			// local alignment
			Matrix[0][j].s = 0;
			Matrix[0][j].d = 0;
			Matrix[0][j].i = 0;
		}
	}
	return;
}

// pretty print the alignment output
void nextLine(char* firstLine, char* secondLine, char* middleLine){
	int count1 = 1, count2 = 1;
	int i;
	int firstPos = 0, secondPos = 0, middlePos = 0;
	int j = 0;
	while(secondLine[secondPos] != 0 && firstLine[firstPos] != 0){
		i = 0;
		printf("s1: %05d ", count1);
		for (i = 0; i < 60 && firstLine[firstPos] != 0; i++) {
			printf("%c", firstLine[firstPos]);
			if( firstLine[firstPos] != '-')
				count1++;
			firstPos++;
		}
		printf(" %d", count1 - 1);
		printf("\n");
		printf("\t  ");
		i = 0;
		for (i = 0; i < 60 && middleLine[middlePos] != 0; i++) {
			printf("%c", middleLine[middlePos]);
			middlePos++;
		}
		printf("\n");

		printf("s2: %05d ", count2);
		i = 0;
		for (i = 0; i < 60 && secondLine[secondPos] != 0; i++) {
			printf("%c", secondLine[secondPos]);
			if( secondLine[secondPos] != '-' )
				count2++;
			secondPos++;
		}
		printf(" %d", count2 - 1);
		printf("\n");
		j++;
	}
	return;
}

// read sequences from fasta file
int readinput(char *inputPath){
	
	char c, line[256], *pos;
	struct stat st;

	if (stat(inputPath, &st) < 0){
		printf("Error reading file\n");
		return -1;
	}

	charPtr = (char *)malloc(st.st_size * sizeof(char));

	if (charPtr == 0){
		printf("Error allocating memory!");
		return -1;
	}

	pos = charPtr;

	FILE *fp = fopen(inputPath, "r");
	if (!fp){
		printf("Error reading file\n");
		return -1;
	} 
	else {
		while((c = fgetc(fp)) != EOF){
			switch(c){
				case '>':
					if (countStr == 0) 
						gene1 = pos; // sequence 1 name
					else{
						*pos = 0; 
						pos ++;
						gene2 = pos; // sequence 2 name
					}

					fgets(line, sizeof(line), fp);
					char *token = strtok(line, " ");
					strcpy(pos, token);
					pos += strlen(token);
					*pos = 0;
					pos++;
					if (countStr == 0){
						s1 = pos; // start of sequence 1 
					} else
						s2 = pos; // start of sequence 2
					countStr++;
					break;

				//case 'a':
				case 'A':
				//case 't':
				case 'T':
				//case 'g':
				case 'G':
				//case 'c':
				case 'C':				
					*pos = c;
					pos++;
					break; // keep consuming the sequence
				default:
					break;
			} 
		}

		*pos = 0;
	}
	if(fp)
		fclose(fp);
	return 0;
}

// process the command line arguments
int processArguments(const char *argv[]){
	
	char *configPath, *algo;
	char paramLine[20],inputPath[64];

	if(argv[1] && argv[2] && (*argv[2] == '0' || *argv[2] == '1')){		
		strcpy(algo, argv[2]);
		SW = atoi(algo);
		strncpy(inputPath, argv[1], 64);
	} 
	else
		return -1;

	if (readinput(inputPath) < 0)
		return -1;

	if(argv[3])
		strcpy(configPath, argv[3]);
	else
		strcpy(configPath, "parameters.config");

	printf("Algorithm: ");
	if (!SW)
		printf("Needleman-Wunsch\n");
	else
		printf("Smith-Waterman\n");

	printf("\n");

	FILE *fp = fopen(configPath, "r");

	if (fp) {
		while(fgets(paramLine, 20, fp)){ // maximum 10 characters required
			char *token = strtok(paramLine, " \t\n");
			if(strcmp(token, "match") == 0){
				token = strtok(NULL, "\t\n");
				ma = atoi(token);
			}
			else if(strcmp(token, "mismatch") == 0){
				token = strtok(NULL, " \t\n");
				mi = atoi(token);
			}
			else if(strcmp(token, "h") == 0){
				token = strtok(NULL, " \t\n");
				h = atoi(token);
			}
			else if(strcmp(token, "g") == 0){
				token = strtok(NULL, " \t\n");
				g = atoi(token);
			}
		}
		printf("Scoring Parameters: Match = %d, Mismatch = %d, h = %d, g = %d\n\n", ma, mi, h, g);
	}

	fclose(fp);
	printf("Sequence 1 = %s\nLength = %d characters\nSequence 2 = %s\nLength = %d characters\n\n",
			gene1, (int)strlen(s1), gene2, (int)strlen(s2));
	return 0;
}

// alignment algo for the 2 sequences
int align(char *s1, char *s2){

	int m = strlen(s1);
	int n = strlen(s2);
	int i, j;

	// allocate the matrix
	if (allocate(m+1, n+1) < 0)
		return -1;		

	// Initialise Row 1 and Column 1
	initialiseMatrix(m, n);

	for (i = 1; i <= m; i++){
		for (j = 1; j <= n; j++){
			
			if (SW) {
				// substitution
				Matrix[i][j].s = substitution(s1, s2, i, j) > 0 ? substitution(s1, s2, i, j) : 0;
				// deletion
				Matrix[i][j].d = (max(Matrix[i-1][j].s + h + g, Matrix[i-1][j].i + h + g, Matrix[i-1][j].d + g)) > 0 ? (max(Matrix[i-1][j].s + h + g, Matrix[i-1][j].i + h + g, Matrix[i-1][j].d + g)) : 0;
				// insertion
				Matrix[i][j].i = (max(Matrix[i][j-1].s + h + g, Matrix[i][j-1].i + g, Matrix[i][j-1].d + h + g)) > 0 ? (max(Matrix[i][j-1].s + h + g, Matrix[i][j-1].i + g, Matrix[i][j-1].d + h + g)) : 0;
			}
			else {
				// substitution
				Matrix[i][j].s = substitution(s1, s2, i, j);				
				// deletion
				Matrix[i][j].d = max(Matrix[i-1][j].s + h + g, Matrix[i-1][j].i + h + g, Matrix[i-1][j].d + g);
				// insertion
				Matrix[i][j].i = max(Matrix[i][j-1].s + h + g, Matrix[i][j-1].i + g, Matrix[i][j-1].d + h + g);
			}
		}
	}
	if (SW){

		// Contribution of Waterman to this field is the ZERO!!
		for (i = 1; i <= m; i++){
			for (j = 1; j <= n; j++){
				
				// Storing the index where the max score was encountered
				if (chooseDirection(i,j) > chooseDirection(maxRowIndex, maxColIndex)){
					maxRowIndex = i;
					maxColIndex = j;
				}
			}
		}
		// Local alignment optimal score
		return chooseDirection(maxRowIndex, maxColIndex);
	}
	// Global alignment optimal score
	return chooseDirection(m,n);
}

// retrace to form the alignment strings
int retrace(int score){
	
	int i , j;
	
	// start retrace from
	if(SW){
		// local alignment
		i = maxRowIndex;
		j = maxColIndex;
	}
	else{
		// global alignment
		i = strlen(s1);
		j = strlen(s2);
	}

	int ctr = 0, total, penalty, moveTo, numMa = 0, numMi = 0, numGa = 0, numOp = 0;		
	char str1[i + j], str2[i + j], str0[i + j];	//str1 to store sequence 1, str2 to store sequence 2, str0 to store '-'/'|'/' '	

	
	if(max(Matrix[i][j].s, Matrix[i][j].d, Matrix[i][j].i) == Matrix[i][j].s)
		moveTo = 1;
	else if(max(Matrix[i][j].s, Matrix[i][j].d, Matrix[i][j].i) == Matrix[i][j].d)
		moveTo = 2;
	else
		moveTo = 0;

	while(i > 0 || j > 0){
		if(SW){
			if(chooseDirection(i,j) == 0)
				break;
		}
		
		// deletion case
		if(moveTo == 2){
			// deletion case
			if (Matrix[i][j].d == (Matrix[i-1][j].d + g)){
				moveTo = 2;
			}
			// substitution case
			else if (Matrix[i][j].d == (Matrix[i-1][j].s + h + g)){
				moveTo = 1;
				numOp++;
			}
			// insertion case
			else {
				moveTo = 0;
				numOp++;
			}
			i--; // 1 char from sequence 1 is consumed
			str1[ctr] = s1[i];
			str2[ctr] = '-';
			str0[ctr] = ' ';
			numGa++;

		} 
		// substitution case
		else if(moveTo == 1){

			if(s1[i-1] == s2[j-1]){
				penalty = ma;
				numMa++;
			} 
			else{
				penalty = mi;
				numMi++;
			}
			// deletion
			if (Matrix[i][j].s == (Matrix[i-1][j-1].d + penalty)){
				moveTo = 2;
			}
			// substitution case
			else if (Matrix[i][j].s == (Matrix[i-1][j-1].s + penalty)){
				moveTo = 1;
			}
			// insertion case
			else {
				moveTo = 0;
			}
			i--; // 1 char from sequence 1 is consumed
			j--; // 1 char from sequence 2 is consumed
			str1[ctr] = s1[i];
			str2[ctr] = s2[j];
			if(penalty == ma)
				str0[ctr] = '|'; 
			else
				str0[ctr] = ' ';
		} 
		// insertion case
		else if(moveTo == 0){
			// deletion case
			if (Matrix[i][j].i == (Matrix[i][j-1].d + g + h)){					
				moveTo = 2;
				numOp++;
			}
			// substitution case
			else if (Matrix[i][j].i == (Matrix[i][j-1].s + g + h) ){					
				moveTo = 1;
				numOp++;
			}
			// insertion case
			else {
				moveTo = 0;
			}
			j--; // char from sequence 2 is consumed
			str1[ctr] = '-';
			str2[ctr] = s2[j];
			str0[ctr] = ' ';
			numGa++;
		}
		else
			return -1;
		
		ctr ++;
	}

	str1[ctr] = 0;
	str0[ctr] = 0;
	str2[ctr] = 0;

	// reverse the strings because they were read in reverse order
	stringReverse(str1);
	stringReverse(str0);
	stringReverse(str2);

	// print them with 60 chars in each line
	nextLine (str1, str2, str0);
	total = numMa + numMi + numGa;

	printf("\nScore: %d\nNumber of: Matches = %d, Mismatches = %d, Gaps = %d, Openings = %d\nIdentities = %d/%d \(%d\%%), Gaps = %d/%d \(%d\%%)\n",
			score, numMa, numMi, numGa, numOp, numMa, total, (numMa * 100 / total), numGa, total, (numGa * 100 / total));
	return 0;
}


int main(int argc, const char *argv[])
{
	int optScore, i;

	if (argc < 1){
		printf("Error parsing arguments!");
		return -1;
	}
	if (processArguments(argv)){
		printf("Error parsing arguments!");
		return -1;
	}

	// Optimal score from alignment
	optScore = align(s1, s2);

	// Forward Computation sucks
	if (optScore == -1){
		printf("You know nothing Jon Snow");
		return -1;
	}

	// Generating the alignment 
	retrace(optScore);

	// Deallocating the Matrix
	for (i = 0; i < (int)strlen(s1) + 1; i++){
			free(Matrix[i]);
		}
		free(Matrix);

	// Pat on the back
	return 0;
}