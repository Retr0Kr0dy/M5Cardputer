Fork edit : ported lib for esp-idf getting rid of arduino.h so no need for arduino as an esp components

### Usage :

Go to your porjects `main` folder, slap the lib here :

```sh
cd main
git clone https://github.com/Retr0Kr0dy/M5Cardputer
```

Go to the root of your project and add m5unified as deps :

```cpp
idf.py add-dependency "m5stack/m5unified^0.2.0" 
```

Then you can simply include headers :

```cpp
#include "M5Cardputer.h"
```

# M5Cardputer

## Basic library for M5Stack M5Cardputer Board 

License
----------------
M5GFX : [MIT](https://github.com/m5stack/M5GFX/blob/master/LICENSE)  
M5Unified : [MIT](https://github.com/m5stack/M5Unified/blob/master/LICENSE)  
 
