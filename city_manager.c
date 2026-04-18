#include <stdio.h>
#include <time.h>

#define MAX_NAME 50
#define MAX_CATEGORY 50
#define MAX_DESC 256

typedef struct{

	int id;
	char inspector[MAX_NAME];
	double latitude;
	double longitude;
	char category[MAX_CATEGORY];
	int severity;
	time_t timestamp;
	char description[MAX_DESC];

}Report;

int main(){
	printf("Report struct done\n");
	return 0;
}
