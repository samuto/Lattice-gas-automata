#include "Python.h"
#include "numpy/arrayobject.h"

long move(long n, long e, long s, long w) {
  long ret = 0;
  if(n % 2 == 1) ret += 1;
  if((e>>1) % 2 == 1) ret += 2;
  if((s>>2) % 2 == 1) ret += 4;
  if((w>>3) % 2 == 1) ret += 8;
  return ret;
}

long reverse(long x) {
  return (x << 2) + (x >> 2);
}

int on_border(int W, int H, int x, int y) {
  if (x == 0 || y == 0 || x == W-1 || y == H-1) {
    return 1;
  }

  return 0;
}

int on_corner(int W, int H, int x, int y) {
  if ((x == 0 || x == W-1) && (y == 0 || y == H-1)) {
    return 1;
  }

  return 0;
}

int which_corner(int W, int H, int x, int y) {
  if (y == 0 && x == 0) return 1;
  if (y == 0 && x == W-1) return 2;
  if (y == H-1 && x == W-1) return 3;
  if (x == 0 && y == H-1) return 4;
  fprintf(stderr, "unexpected in which_corner");
  return 0;
}

int which_border(int W, int H, int x, int y) {
  if (y == 0) return 1;
  if (x == W-1) return 2;
  if (y == H-1) return 3;
  if (x == 0) return 4;
  fprintf(stderr, "unexpected in which_border");
  return 0;
}


static PyObject * c_module(PyObject *self, PyObject *args) {
    PyObject *array_python_object;
    PyObject *array_python_object_temp;

    if (!PyArg_ParseTuple(args, "OO",
            &array_python_object,
            &array_python_object_temp)) {
        fprintf(stderr, "Failed to parse arguments.\n");
        Py_RETURN_NONE;
    }

    PyArrayObject *array;
    PyArrayObject *array_temp;
    array = (PyArrayObject *)PyArray_ContiguousFromAny(array_python_object, PyArray_LONG, 2, 2);
    array_temp = (PyArrayObject *)PyArray_ContiguousFromAny(array_python_object_temp, PyArray_LONG, 2, 2);
    if (array == NULL || array_temp == NULL) {
        fprintf(stderr, "Invalid array object.\n");
        /* make sure we remove our refererences to the python objects we have before returning. */
        Py_DECREF(array_python_object);
        Py_DECREF(array_python_object_temp);
        Py_RETURN_NONE; /* returns None to the Python side, of course. */
    }

    // read from array, write to array_temp
    // collision step
    int iRow, iCol, corner, border;
    int H = array->dimensions[0];
    int W = array->dimensions[1];
    for (iRow = 0; iRow < H; ++iRow) {
        for (iCol = 0; iCol < W; ++iCol) {
            long *data = PyArray_GETPTR2(array, iRow, iCol);
            long *data_temp = PyArray_GETPTR2(array_temp, iRow, iCol);

            // cell collision
            if (!on_border(H, W, iRow, iCol)) {
              if (*data == 5) {
                (*data_temp) = 10;
              } else if (*data == 10) {
                (*data_temp) = 5;
              } else {
                (*data_temp) = *data;
              }
            } else { // on border or corner
              (*data_temp) = reverse(*data);
            }
        }
    }

    // read from array_temp, write to array
    // move step
    for (iRow = 0; iRow < H; ++iRow) {
        for (iCol = 0; iCol < W; ++iCol) {
            long *data = PyArray_GETPTR2(array, iRow, iCol);

            if (!on_border(H, W, iRow, iCol)) {
              long *n = PyArray_GETPTR2(array_temp, iRow-1, iCol);
              long *e = PyArray_GETPTR2(array_temp, iRow, iCol+1);
              long *s = PyArray_GETPTR2(array_temp, iRow+1, iCol);
              long *w = PyArray_GETPTR2(array_temp, iRow, iCol-1);
              (*data) = move(*n,*e,*s,*w);
            } else { // on border
              if (on_corner(H, W, iRow, iCol)) {
                corner = which_corner(H, W, iRow, iCol);
                if (corner == 1) { // NW
                  long *e = PyArray_GETPTR2(array_temp, iRow, iCol+1);
                  long *s = PyArray_GETPTR2(array_temp, iRow+1, iCol);
                  (*data) = move(0,*e,*s,0);
                } else if (corner == 2) { // NE
                  long *s = PyArray_GETPTR2(array_temp, iRow+1, iCol);
                  long *w = PyArray_GETPTR2(array_temp, iRow, iCol-1);
                  (*data) = move(0,0,*s,*w);
                } else if (corner == 3) { // SE
                  long *n = PyArray_GETPTR2(array_temp, iRow-1, iCol);
                  long *w = PyArray_GETPTR2(array_temp, iRow, iCol-1);
                  (*data) = move(*n,0,0,*w);
                } else { // SW
                  long *n = PyArray_GETPTR2(array_temp, iRow-1, iCol);
                  long *e = PyArray_GETPTR2(array_temp, iRow, iCol+1);
                  (*data) = move(*n,*e,0,0);
                }
              } else { // on border, not on corner
                border = which_border(H, W, iRow, iCol);
                if (border == 1) { // north
                  long *e = PyArray_GETPTR2(array_temp, iRow, iCol+1);
                  long *s = PyArray_GETPTR2(array_temp, iRow+1, iCol);
                  long *w = PyArray_GETPTR2(array_temp, iRow, iCol-1);
                  (*data) = move(0,*e,*s,*w);
                } else if (border == 2) { // east
                  long *n = PyArray_GETPTR2(array_temp, iRow-1, iCol);
                  long *s = PyArray_GETPTR2(array_temp, iRow+1, iCol);
                  long *w = PyArray_GETPTR2(array_temp, iRow, iCol-1);
                  (*data) = move(*n,0,*s,*w);
                } else if (border == 3) { // south
                  long *n = PyArray_GETPTR2(array_temp, iRow-1, iCol);
                  long *e = PyArray_GETPTR2(array_temp, iRow, iCol+1);
                  long *w = PyArray_GETPTR2(array_temp, iRow, iCol-1);
                  (*data) = move(*n,*e,0,*w);
                } else { // west
                  long *n = PyArray_GETPTR2(array_temp, iRow-1, iCol);
                  long *e = PyArray_GETPTR2(array_temp, iRow, iCol+1);
                  long *s = PyArray_GETPTR2(array_temp, iRow+1, iCol);
                  (*data) = move(*n,*e,*s,0);
                }
              }
            }
        }
    }

    /* this is just like DECREF, but doesn't crash if the argument is NULL. */
    Py_XDECREF(array);
    Py_XDECREF(array_temp);

    //Py_DECREF(list_object);
    //Py_DECREF(array_python_object);
    Py_RETURN_NONE;
}

static PyMethodDef C_Module_Methods[] = {
    /* function name, function pointer, always METH_VARARGS (could be KEYWORDS too), documentation string. */
    { "c_module", c_module, METH_VARARGS, "C module example function." },
    { NULL, NULL, 0, NULL } /* list terminator. */
};

PyMODINIT_FUNC
initc_module(void) {
    (void)Py_InitModule("c_module", C_Module_Methods);
    import_array(); /* Numpy magic. always call this if using numpy. */
}