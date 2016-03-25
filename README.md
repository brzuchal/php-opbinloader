Zend OPcache Binary Loader
==========================

## Build

After cloning hit:

```bash
phpize
./configure
make
```

Then run simple test for loading with PHP:

```bash
php -n -d extension_dir=./modules -d extension=opbinloader.so -m
```

> You should notice `opbinloader` in modules.

You can also get extension information with:

```bash
php -n -d extension_dir=./modules -d extension=opbinloader.so --re opbinloader
```

Extension information:

```
Extension [ <persistent> extension #39 opbinloader version 1.0 ] {

  - Functions {
    Function [ <internal:opbinloader> function opcache_compile ] {

      - Parameters [2] {
        Parameter #0 [ <required> $source ]
        Parameter #1 [ <required> $destination ]
      }
    }
  }
}
```

## License

The MIT License (MIT)

Copyright (c) 2016 Michał Brzuchalski (aka brzuchal) <michal.brzuchalski@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.