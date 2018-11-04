#include <bits/stdc++.h>
#include "../WFMPMC.h"

using namespace std;

class Foo
{
public:
    // Note: no default constructor here

    Foo(int v)
        : a(v) {
        ctor++;
    }

    Foo(Foo&& other)
        : a(other.a) {
        ctor++;
    }

    Foo& operator=(Foo&& other) {
        a = other.a;
    }

    ~Foo() {
        dtor++;
    }

    int a;
    static int ctor;
    static int dtor;
};

int Foo::ctor = 0;
int Foo::dtor = 0;

int ctor = 0;
int dtor = 0;
void assertTor() {
    assert(ctor == Foo::ctor && dtor == Foo::dtor);
}

void torTest() {
    WFMPMC<Foo, 8> q;
    assertTor();
    {
        auto idx = q.getWriteIdx();
        Foo* data;
        while((data = q.getWritable(idx)) == nullptr)
            ;
        assertTor();
        new(data) Foo(1);
        q.commitWrite(idx);
    }
    ctor++;
    assertTor();
    {
        while(!q.tryPush(2))
            ;
    }
    ctor++;
    assertTor();
    {
        while(!q.tryVisitPush(([](Foo& data) {
            assertTor();
            new(&data) Foo(3);
            ctor++;
            assertTor();
        })))
            ;
    }
    assertTor();

    q.emplace(4);

    ctor++;
    assertTor();

    {
        int64_t idx = q.getReadIdx();
        Foo* data;
        while((data = q.getReadable(idx)) == nullptr)
            ;
        assertTor();
        assert(data->a == 1);
        q.commitRead(idx);
    }
    dtor++;
    assertTor();
    {
        Foo data(0);
        ctor++;
        assertTor();
        while(!q.tryPop(data))
            ;
        assert(data.a == 2);
        dtor++;
        assertTor();
    }
    dtor++;
    assertTor();
    {
        while(!q.tryVisitPop([](Foo&& data) {
            assert(data.a == 3);
            assertTor();
        }))
            ;
    }
    dtor++;
    assertTor();

    q.emplace(5);

    ctor++;
    assertTor();

    assert(q.pop().a == 4);
    ctor++;
    dtor++;
    dtor++;
    assertTor();
}

int main() {
    assert(Foo::ctor == Foo::dtor);
    torTest();
    assert(Foo::ctor == Foo::dtor);

    cout << "test passed" << endl;
    return 0;
}

