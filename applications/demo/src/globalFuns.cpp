#include <globalFuns.h>

using namespace std;

std::vector<char> readSpirvFile(const std::string& filename) {
	// using ios:ate FLAG to retrieve fileSize ; _Prot default is set to 64
	ifstream currentFile(filename, ios::binary | ios::ate);

	// ifstream converts object currentFile to TRUE if the file was opened successfully
	if (currentFile) {
		// size_t guarantees to hold any array index => here tellg and ios::ate helps to get the size of the file
		size_t currentFileSize = (size_t)currentFile.tellg();
		vector<char> buffer(currentFileSize);

		// sets reading head to the beginning of the file
		currentFile.seekg(0);
		// reads ENTIRE binary into RAM for the program
		currentFile.read(buffer.data(), currentFileSize);
		// close will be called by destructor at the end of the scope but this line helps to understand the code
		currentFile.close();

		return buffer;
	}
	else {
		throw runtime_error("Failed to read shaderfile into RAM!");
	}
}