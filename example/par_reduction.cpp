//#include <my_parallel>
#include "../include/par_reduction.hpp"



void f1(int a, int* b, int c)
{ *b += a * c; }

void f2(int a, int* b)
{ *b += a; }

template<typename T>
requires my_parallel::is_number<T>
void f3(T a, T* b, T c)
{ *b += a * c; }

void f4(int x, int *y)
{ *y *= x; }

void f5(std::tuple<int,int> x, std::tuple<int,int> *y)
{
    std::get<0>(*y) += std::get<0>(x) * 2;
    std::get<1>(*y) += std::get<1>(x);
}

int main(int argc, char* argv[])
{
    std::vector<std::tuple<int,int>> v;
    const std::size_t size = 10;
    const std::size_t num_threads = 17;

    for(auto i = 1; i <= size; i++)
        v.push_back(std::tuple<int,int>(1,1));

    int x = 0;

    std::tuple<int,int> r2 = my_parallel::reduce(v.begin(), v.end(), std::tuple<int,int>(0,0), &f5);

    std::cout << "Reduction: " << std::get<0>(r2) << " " << std::get<1>(r2) << std::endl;

}