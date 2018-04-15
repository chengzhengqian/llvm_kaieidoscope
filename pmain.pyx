cdef extern from "parse.h":
    int getToken()


def get_tok():
    print(getToken())
