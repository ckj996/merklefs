/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "../lib/fetcher.hpp"

#include <iostream>
#include <string>

using namespace std;

int main(int argc, char **argv)
{
    string url = "unix:///tmp/object-fetcher.sock";
    string key = "hello";

    if (argc > 1)
    {
        key = argv[1];
    }

    Fetcher fetcher(url);
    bool reply = fetcher.fetch(key);
    cout << "Fetcher received: " << (reply ? "OK" : "BAD") << endl;

    return 0;
}