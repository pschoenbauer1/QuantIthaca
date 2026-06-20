import quantithaca as qi

def test_add():
    assert qi.add(10, 7) == 17

def test_nb_callback():
    res = qi.cpp_callback("test_callback", "What's up")
    assert(res == "Hello C++! Python Here! You said What's up.")
    print(res)