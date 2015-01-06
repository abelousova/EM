#include "IBMModel.h"

int main() {
	IBMModel model("alignment-en-fr.txt", "output.txt");
	model.execute();	
	return 0;
}