/* Canned codegen quality test sequences
(C) 2017-2019 Niall Douglas <http://www.nedproductions.biz/> (8 commits)


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
    (See accompanying file Licence.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

#include "../../include/boost/outcome.hpp"

extern QUICKCPPLIB_NOINLINE std::exception_ptr test1(std::exception_ptr ec)
{
  using namespace boost::outcome;
  outcome<int> m1(std::move(ec));
  outcome<int> m2(std::move(m1));
  return m2.get_exception();
}
extern QUICKCPPLIB_NOINLINE void test2()
{
}

int main(void)
{
  int ret=0;
  if(std::exception_ptr()!=test1(std::exception_ptr())) ret=1;
  test2();
  return ret;
}
