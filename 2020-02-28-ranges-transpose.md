---
layout: post
title: "Writing A Transpose Function With Range-v3"
date: 2020-04-20
author: Bradley Bauer
---

In this post we will work through a few examples of using range-v3.
I'll be using range-v3 because C++20's std::ranges does not include all the view adaptors used in these examples. Specifically, chunk and stride are not in C++20 according to cppreference.

All the code in this post is on [godbolt](https://godbolt.org/z/uMmq8f) if you'd like to take a closer look. If you want to compile on your own machine then you can get started with
```
git clone https://github.com/xdaimon/ranges_blog
cd ranges_blog
git clone https://github.com/ericniebler/range-v3
./build.sh && ./a.out
```

There is already a bunch of great introductions to ranges so I will give just a very brief overview here. A range is a colleciton of things you can iterate over. A view is like a transformed image of some other range. A view is cheap to copy since it does not store its viewed range. Finally, a view adaptor is something that takes a range and produces a view of that range.

Ok, let's see some view adaptors useful for computing the matrix transpose. I think that given the inputs, outputs, and names of the adaptors, you will be able to figure out for yourself what is happening here.

```cpp
#include <bits/stdc++.h>
#include <range/v3/all.hpp>
namespace rs = ranges;
using namespace ranges::views;

// ...

auto print=[](auto rng){std::cout<<rng<<std::endl;};
auto print2D=[&](auto rng){for(auto r:rng)print(r);};

auto x = ints(1,5+1);
print(x); // [1,2,3,4,5]
print(x | drop(2)); // [3,4,5]
print(x | stride(2)); // [1,3,5]
print(x | transform([](auto xi){ return 2*xi; })); // [2,4,6,8,10]
print2D(x | chunk(2)); // [1,2]
                       // [3,4]
                       // [5]
print(x | chunk(2) | join); // [1,2,3,4,5]

auto y = std::vector{ 1, 2, 3, 4 };
print(rs::inner_product(x, y, -.5)); // 29.5
std::cout << rs::distance(y) << std::endl; // 4
```

We can use transform and inner_product to compute the matrix-vector product Wx.
```cpp
auto W = ints(1,2*5+1) | chunk(5);
print(W | transform([&](auto r){ return rs::inner_product(r, x, 0); })); // [55,130]
```

Computing the matrix product XW, with X and W both a range of rows, is more involved.
One way to do it is to use inner_product between the rows of X and columns of W. This would be easy if we had a range over the columns of W.
But, to get such a range we need to compute the transpose of W.
```cpp
W = ints(1,3*2+1) | chunk(2);
print2D(ints(0,2) | transform([&](int i) { // for each column
  return W | join // concatenate all the rows into a single range
           | drop(i) // remove everything before the 1st element of the ith column
           | stride(2); // take every Nth item to provide a view of the ith column
}));
```

Let's put that into a function,

```cpp
auto transpose = [](auto rng) {
  auto flat = rng | join;
  int height = rs::distance(rng);
  int width = rs::distance(flat) / height;
  return ints(0,width) | transform([=](int i) {
    return flat | drop(i) | stride(width);
  });
};
print2D(transpose(W));
```
The inner lambda captures by value because it does not execute until after the rng variable has been destroyed.

Unfortunately, this does not compile. Clang gives errors that suggest `flat` does not satisfy the range concept. We can check this directly using a static_assert in the lambda.i
```cpp
static_assert(!rs::range<decltype(flat)>, "i compile");
```
After some searching I found that the problem is solved by marking the lambda as mutable. Adding the mutable specification causes the lambda to capture as non-const (the default for capture by value is const). I think the compilation error has something to do with join returning an "input_range". An input_range is a type of range that can be iterated over <em>at least</em> once. If we had a const view over an input_range, and the range could be iterated over only a finite number of times, then how would the const view keep track of how many times it has been iterated over? It must keep some internal state and therefore cannot be declared const. This is not much more than a guess though since I have not dived too deep into the library implementation and the documentation is sparse.

Now we can write the product without worring about how exactly to index into X's and W's underlying arrays.
```cpp
auto X = ints(1, 2*3 + 1) | chunk(3);
print2D(X | transform([&](auto xr) {
  return transpose(W) | transform([=](auto wc) {
    return rs::inner_product(xr, wc, 0);
  }); // [22,28]
}));  // [49,64]
```

Let's see an example that highlights an important difference between views and standard library containers.
```cpp
auto intstream = rs::istream_view<int>{std::cin};
for (auto i:intstream)
  std::cout << "In loop: " << i << std::endl;
```
Hmmm... I thought `for (auto i : rng);` was equivalent to something that compared begin and end iterators?
But in the above example, `intstream` cannot have an end iterator!
From what I've gathered [online](https://stackoverflow.com/q/32900557), a view uses a 'sentinel' in place of an end iterator.
A sentinel is like an end iterator but is allowed to have a type that is not an iterator type.
By overriding the equality operator between a sentinel and iterator, it is possible to check whether an iterator is at the end of a range without knowing the position of the last element in the range.
Presumably, view adaptors like `take_while(lambda)` allow you to specify this comparison function directly.

One last note.
If you write code using range-v3 you may find that the compiler's error messages are difficult to understand.
One reason for this is because ranges-v3 emulates concepts through a mixture of macros and template metaprogramming.
So when things fail, the error messages have to do with things deep in the library's implementation.
Hopefully std::ranges will be able to make use of non-emulated concepts to fail in a more graceful way.

In sum, I think ranges are a very nice addition to the standard library. I hope these examples helped you get a feel for what can be done with ranges and how you might use ranges to improve your own code.

Here are a few links I found useful while learning about ranges.
  * [C++ code samples before and after Ranges](https://mariusbancila.ro/blog/2019/01/20/cpp-code-samples-before-and-after-ranges/)
  * [A beginner's guide to C++ Ranges and Views](https://hannes.hauswedell.net/post/2019/11/30/range_intro/)
  * [Tutorial: Writing your first view from scratch](https://hannes.hauswedell.net/post/2018/04/11/view1/)
  * [The Surprising Limitations of C++ Ranges Beyond Trivial Cases](https://www.fluentcpp.com/2019/09/13/the-surprising-limitations-of-c-ranges-beyond-trivial-use-cases/)
  * [The Range-v3 User Manual](https://ericniebler.github.io/range-v3/)



<h1>Extra Stuff</h1>


I did not add this example since I am not sure it is a fair comparison.

    // What does matrix multiplication look like without ranges?
    auto X = ints(1, 4*5 + 1);
    auto W = ints(1, 5*3 + 1);
    auto XW = std::vector<int>(4*3);
    for (int i = 0;  i < 4; ++i)
      for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 5; ++k)
          XW[i*3+j] += X[i*5+k] * W[k*3+j]; // transpose is super easy here

    // However, writing this with raw loops is cumbersome if we add few bells and whistles
    auto X = ints(1,2*4*5+1);
    auto W = ints(1,2*5*3+1);
    auto XW = std::vector<int>(2*4*3);
    for (int b = 0;  b < 2; ++b)
      for (int i = 0;  i < 4; ++i)
        for (int j = 0; j < 3; ++j)
          for (int k = 0; k < 5; ++k)
            XW[b*4*3 + i*3+j] += X[b*4*5 + (3-i)*5+(4-k)] * W[b*5*3 + k*3+j];
    // This code does batch matrix multiplication where X is flipped vertically and horizontally. you can see, the index calculations are much more difficult and error prone than before. While the following ranges code remains easy to write and read.

    // Ranges code that does a similar computation
    auto X = ints(1,2*4*5+1) | chunk(5) | chunk(4);
    auto W = ints(1,2*5*3+1) | chunk(3) | chunk(5);
    auto XW = zip_with([&](auto x, auto w) {
      return x | reverse | transform([w](auto xr) {
        return transpose(w) | transform([xr](auto wc) {
          return rs::inner_product(xr, wc, 0);
        });
      });
    }, X, W);

I also wrote a function with ranges that swaps the first and last dimension of an arbitrary tensor (n dimensional matrix). I did not include it in the rough draft since it is more of the same `drop`,`stride`,`chunk` that I've already shown in `transpose`.

