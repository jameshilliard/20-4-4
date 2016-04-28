//
// Make sure the TwStatus checks work on enums
// 

enum TwStatus {
   A,
   B,
   C
};

TwStatus Foo() {
    return A;
}

void TwStatusTest () {
    TwStatus s = Foo();                      // should not be an error
    (void) s;
    Foo();                                   // should be an error 
}

//
// Make sure the TvStatus checks work on enums with #define and casts
// 

enum TvStatus {};

#define D ((TvStatus)4)
#define E ((TvStatus)5)
#define F ((TvStatus)6)

TvStatus Bar() {
    return D;
}

void TvStatusTest () {
    TvStatus s = Bar();                      // should not be an error
    (void) s;
    Bar();                                   // should be an error 
    (void) Bar();                            // still an error!
}

