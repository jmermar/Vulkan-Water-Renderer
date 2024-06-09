#define SSB(name, struct) layout(binding = 1) buffer S_##name struct _##name[];
#define GET(name) (_##name[name##Bind])