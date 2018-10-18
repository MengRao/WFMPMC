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

void assertTor(int ctor, int dtor) {
    assert(ctor == Foo::ctor && dtor == Foo::dtor);
}

void torTest() {
    WFMPMC<Foo, 8> q;
    assertTor(0, 0);
    {
        auto idx = q.getWriteIdx();
        Foo* data;
        while((data = q.getWritable(idx)) == nullptr)
            ;
        assertTor(0, 0);
        new(data) Foo(1);
        q.commitWrite(idx);
    }
    assertTor(1, 0);
    {
        while(!q.tryEmplace(2))
            ;
    }
    assertTor(2, 0);

    q.emplace(3);

    assertTor(3, 0);

    {
        int64_t idx = q.getReadIdx();
        Foo* data;
        while((data = q.getReadable(idx)) == nullptr)
            ;
        assertTor(3, 0);
        assert(data->a == 1);
        q.commitRead(idx);
    }
    assertTor(3, 1);
    {
        Foo data(0);
        assertTor(4, 1);
        while(!q.tryPop(data))
            ;
        assert(data.a == 2);
        assertTor(4, 2);
    }
    assertTor(4, 3);

    q.emplace(4);

    assertTor(5, 3);

    assert(q.pop().a == 3);

    assertTor(6, 5);
}

int main() {
    assert(Foo::ctor == Foo::dtor);
    torTest();
    assert(Foo::ctor == Foo::dtor);

    cout << "test passed" << endl;
    return 0;
}

