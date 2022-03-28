// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>

using namespace std;

int Couting(int num) {
	int ansver = 0;
	for (int i = 0; i <= num; ++i) {
		if (i < 100 && i == 3 || i / 10 == 3 || i % 10 == 3) {
			++ansver;
		}
		else if (i / 100 == 3 || i % 10 == 3 || (i / 10) % 10 == 3) {
			++ansver;
		}
	}
	return ansver;
}

int main() {
	int number = 1000;
	cout << Couting(number);
}
// Закомитьте изменения и отправьте их в свой репозиторий.
