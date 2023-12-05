int main() {
	char* block = (char*)malloc(sizeof(char) * 3);
	*(block + 0) = 'h';
	*(block + 1) = 'i';
	*(block + 2) = 0;
	puts(block);
}