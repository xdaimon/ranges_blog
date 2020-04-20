#include <bits/stdc++.h>
#include <range/v3/all.hpp>
namespace rs = ranges;
using namespace ranges::views;





// make_view_closure is a helper that creates an object that is pipeable
auto transpose = rs::make_view_closure([](auto rng) {
  auto flat = rng | join;
  int height = rs::distance(rng);
  int width = rs::distance(flat) / height;
  return ints(0,width) | transform([=](int i) mutable {
    return flat | drop(i) | stride(width);
  });
});

// one of the chunks here could be removed by inlining the transpose but meh.
auto transpose4d = rs::make_view_closure([](auto rng) {
  const int height = rs::distance(rng[0]);
  const int width = rs::distance(rng[0][0]);
  const int depth = rs::distance(rng[0][0][0]);
  auto flat = rng|join|join|join;
  return ints(0,depth) | transform([=](int whichSlice) mutable {
    auto sliceRange = flat // [b*h*w*d]
                    | drop(whichSlice) // [b*h*w*d - whichSlice]
                    | stride(depth); // [b*h*w]
    return transpose(
            sliceRange | chunk(height*width) // [b,h*w]
          ) // [h*w,b]
      | chunk(width); // [h,w,b]
  });}); // [d,h,w,b]

auto getColor(int i) {
    return std::to_string(i);
//   return std::string("\033[1;")
//   + std::to_string(31+((i%3)==2?3:(i%3)))
//   + std::string("m")
//   + std::to_string(i)
//   + std::string("\033[0m");
};

auto colorful_ints(int n) {
  return ints(0,n) | transform([&](int i){
    return getColor(i) + std::string(2-int(log10(i-i%10+1)),' ');
  });
};

auto print(auto rng) {
  std::cout<<rng<<std::endl<<std::endl;
}

auto print2D(auto rng) {
  for(auto r:rng)
    std::cout<<r<<std::endl;
  std::cout << std::endl;
}







int main() {
  print("------------- Initial examples -------------");
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

  // we can have const views
  const auto z = ints(0,5) | chunk(2);
  std::cout << z << std::endl;









  print("------------- Matrix transpose -------------");
  auto W = ints(1,2*5+1) | chunk(5);
  print(W | transform([&](auto r){ return rs::inner_product(r, x, 0); }));

  W = ints(1,3*2+1) | chunk(2);
  print2D(ints(0,2) | transform([&](int i) {
    return W | join // concatenate all the rows into a single range
             | drop(i) // shift the 1st element of the ith column to the beginning
             | stride(2); // take every Nth item to provide a view of the ith column
  }));














  print("------------- Matrix Product -------------");
  auto X = ints(1, 2*3 + 1) | chunk(3);
  print2D(X | transform([&](auto xr) {
    return transpose(W) | transform([=](auto wc) {
      return rs::inner_product(xr, wc, 0);
    }); // [22,28]
  }));  // [49,64]
  









  print("------------- Begin/End Fiasco -------------");
  print("Enter integers until you get bored. Then enter something else to exit the loop.");
  std::cout << "istream_view length:" << rs::distance(rs::istream_view<int>{std::cin}) << std::endl; // O(N)
  std::cin.clear();
  std::cin.ignore(10000,'\n');
  auto intstream = rs::istream_view<int>{std::cin};
  print("Do it again.");
  for (auto w:intstream)
    std::cout << "In loop:" << w << std::endl;
  // intstream.end() is a sentenel but instream.begin() is not
  // intstream.end().hello; // uncomment to see the types in the compiler's error output
  // intstream.begin().hello;











  print("------------- 4d 'transpose' -------------");
  constexpr int batch=2,height=4,width=5,depth=3;
  auto T = colorful_ints(batch*height*width*depth) | chunk(depth) | chunk(width) | chunk(height);
  print("A representation of a batch 2 of rgb images.");
  for (auto img : T)
    print2D(img);
  print("And it's 'transpose'");
  for (auto img : T | transpose4d)
    print2D(img);
  
  // // It's a lot easier to do this with raw loops.
  // auto V = colorful_ints(batch*height*width*depth);
  // for (int d = 0; d < depth; ++d){
  // for (int h = 0; h < height; ++h){
  // for (int w = 0; w < width; ++w){
  // for (int b = 0; b < batch; ++b){
  //   std::cout << V[b*height*width*depth + h*width*depth + w*depth + d];
  // }
  // }
  // std::cout << std::endl;
  // }
  // std::cout << std::endl;
  // }


  // --------------- Compiler Error Messages ---------------
  const auto bug = ints(0,10) | chunk(2) | join;
  static_assert(!rs::range<decltype(bug)>, "i compile");
  // std::cout << (bug | all) <<std:: endl;
  // for (auto w:bug) std::cout << "hi" << std::endl; // more clear error message
  // std::cout << rs::inner_product(bug,bug,0) << std::endl;

  // A bug!
  // auto a = concat(ints(0,15), ints(0,15));
  // auto b = repeat_n(ints(0,15), 2);
  // std::cout << concat(a,b) << std::endl;

}