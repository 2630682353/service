#include <iostream>
#include <fstream>
int main()
{
	using namespace std;
	char s1[10] = "hello";
	char s2[10] = "hello";
	string s3 = "hello";
	string s4 = "hello";
	char * s5 = "hello";
	int b = 55;
	int *a = &b;
	char s6[] = "hello";
	cout << (s1 == s2) << "ssss"<< (s3==s4) << endl;
	ofstream outFile;
	outFile.open("./hhh");
	outFile << "this is my first";
	outFile.close();
	cout << "Come up and c++ me some time.";
	cout << endl << sizeof(s3) << sizeof(*s5) << sizeof(s6) << sizeof(a) << s3[1];
	return 0;
}
