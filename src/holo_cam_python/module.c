#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <numpy/arrayobject.h>

#define HOLOCAM_IMPLEMENTATION
#include "..\holo_cam.h"

typedef struct py__Holo_Cam
{
	PyObject_HEAD
	Holo_Cam* cam;
} py__Holo_Cam;

static void
py__HoloCam_Dealloc(py__Holo_Cam* self)
{
	HoloCam_Destroy(&self->cam);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static int
py__HoloCam_Init(py__Holo_Cam* self, PyObject* args, PyObject* kwds)
{
	int result = -1;

	static char* kwlist[] = { "unique_name", "width", "height", "fps", "port", 0	};

	PyObject* uname = 0;
	int width       = 0;
	int height      = 0;
	int fps         = 0;
	int port        = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "Uiiii", kwlist, &uname, &width, &height, &fps, &port))
	{
		//// ERROR
		result = -1;
	}
	else
	{
		if (width < 0 || width >= (1 << 16))
		{
			//// ERROR
			PyErr_SetString(PyExc_ValueError, "width must be in range(0, 65536)");
			result = -1;
		}
		else if (height < 0 || height >= (1 << 16))
		{
			//// ERROR
			PyErr_SetString(PyExc_ValueError, "height must be in range(0, 65536)");
			result = -1;
		}
		else if (fps < 0 || fps >= (1 << 8))
		{
			//// ERROR
			PyErr_SetString(PyExc_ValueError, "fps must be in range(0, 256)");
			result = -1;
		}
		else if (port < 0 || port >= (1 << 16))
		{
			//// ERROR
			PyErr_SetString(PyExc_ValueError, "port must be in range(0, 65536)");
			result = -1;
		}
		else
		{
			wchar_t* name = PyUnicode_AsWideCharString(uname, 0);
			if (name == 0)
			{
				//// ERROR
				result = -1;
			}
			else
			{
				Holo_Cam* cam = HoloCam_Create(name, (uint16_t)width, (uint16_t)height, (uint8_t)fps, (uint16_t)port);

				if (cam == 0)
				{
					//// ERROR
					result = -1;
					PyErr_SetString(PyExc_RuntimeError, "failed to create camera");
				}
				else
				{
					HoloCam_Destroy(&self->cam);
					self->cam = cam;

					result = 0;
				}
			}

			PyMem_Free(name);
		}
	}

	return result;
}

static PyObject*
py__HoloCam_Present(py__Holo_Cam* self, PyObject* args)
{
	PyObject* frame = 0;

	if (!PyArg_ParseTuple(args, "O", &frame))
	{
		//// ERROR
	}
	else
	{
		if (!PyArray_Check(frame))
		{
			//// ERROR
			PyErr_SetString(PyExc_TypeError, "frame must be a numpy array");
		}

		npy_intp dims[1] = { self->cam->width * self->cam->height };
		uint32_t* data = 0;
		int i = PyArray_AsCArray(&frame, &data, dims, 1, PyArray_DescrFromType(NPY_UINT32));

		if (HoloCam_Present(self->cam, data)) Py_RETURN_TRUE;
		else                                  Py_RETURN_FALSE;
	}

	return 0;
}

static PyTypeObject py__HoloCamType = {
	PyVarObject_HEAD_INIT(0, 0)
	.tp_name = "holocam.HoloCam",
	.tp_basicsize = sizeof(py__Holo_Cam),
	.tp_flags     = Py_TPFLAGS_DEFAULT,
	.tp_doc       = PyDoc_STR("holocam camera object"),
	.tp_new       = PyType_GenericNew,
	.tp_dealloc   = (destructor)py__HoloCam_Dealloc,
	.tp_init      = (initproc)py__HoloCam_Init,
	.tp_methods   = (PyMethodDef[]){
		{ "present", (PyCFunction)py__HoloCam_Present, METH_VARARGS, "send provided frame to the camera when it is ready to recieve the next frame" },
		{0}
	},
};

typedef struct py__Holo_Camera_Reader
{
	PyObject_HEAD
	Holo_Camera_Reader* reader;
} py__Holo_Camera_Reader;

static void
py__HoloCameraReader_Dealloc(py__Holo_Camera_Reader* self)
{
	HoloCameraReader_Destroy(&self->reader);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static int
py__HoloCameraReader_Init(py__Holo_Camera_Reader* self, PyObject* args, PyObject* kwds)
{
	int result = -1;

	static char* kwlist[] = { "symbolic_name", "width", "height", "fps", 0 };

	PyObject* uname = 0;
	int width       = 0;
	int height      = 0;
	int fps         = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "Uiii", kwlist, &uname, &width, &height, &fps))
	{
		//// ERROR
		result = -1;
	}
	else
	{
		if (width < 0 || width >= (1 << 16))
		{
			//// ERROR
			PyErr_SetString(PyExc_ValueError, "width must be in range(0, 65536)");
			result = -1;
		}
		else if (height < 0 || height >= (1 << 16))
		{
			//// ERROR
			PyErr_SetString(PyExc_ValueError, "height must be in range(0, 65536)");
			result = -1;
		}
		else if (fps < 0 || fps >= (1 << 8))
		{
			//// ERROR
			PyErr_SetString(PyExc_ValueError, "fps must be in range(0, 256)");
			result = -1;
		}
		else
		{
			wchar_t* name = PyUnicode_AsWideCharString(uname, 0);
			if (name == 0)
			{
				//// ERROR
				result = -1;
			}
			else
			{
				Holo_Camera_Reader* reader = HoloCameraReader_Create(name, (uint16_t)width, (uint16_t)height, (uint8_t)fps);

				if (reader == 0)
				{
					//// ERROR
					result = -1;
					PyErr_SetString(PyExc_RuntimeError, "failed to create camera reader");
				}
				else
				{
					HoloCameraReader_Destroy(&self->reader);
					self->reader = reader;

					result = 0;
				}
			}

			PyMem_Free(name);
		}
	}

	return result;
}

static PyObject*
py__HoloCameraReader_ReadFrame(py__Holo_Camera_Reader* self, PyObject* args)
{
	PyObject* result = 0;

	npy_intp dims[1] = { self->reader->width*self->reader->height };
	PyObject* array = PyArray_SimpleNew(1, dims, NPY_UINT32);
	if (array != 0)
	{
		uint32_t* data = PyArray_DATA(array);
		HoloCameraReader_ReadFrame(self->reader, data);

		result = array;
	}

	return result;
}

static PyTypeObject py__HoloCameraReaderType = {
	PyVarObject_HEAD_INIT(0, 0)
	.tp_name = "holocam.HoloCameraReader",
	.tp_basicsize = sizeof(py__Holo_Camera_Reader),
	.tp_flags     = Py_TPFLAGS_DEFAULT,
	.tp_doc       = PyDoc_STR("holocam camera reader object"),
	.tp_new       = PyType_GenericNew,
	.tp_dealloc   = (destructor)py__HoloCameraReader_Dealloc,
	.tp_init      = (initproc)py__HoloCameraReader_Init,
	.tp_methods   = (PyMethodDef[]){
		{ "read_frame", (PyCFunction)py__HoloCameraReader_ReadFrame, METH_NOARGS, "request and read one frame from camera" },
		{0}
	},
};

static void
py__Holo_Destroy(void* _)
{
	Holo_Cleanup();
}

static PyObject*
py__Holo_GetCameraNames(PyObject* self, PyObject* args)
{
	PyObject* result = 0;

	unsigned int names_len  = 0;
	Holo_Camera_Name* names = Holo_GetCameraNames(&names_len);

	if (names == 0)
	{
		//// ERROR
		PyErr_SetString(PyExc_RuntimeError, "failed to get camera names");
	}
	else
	{
		bool encountered_errors = false;

		PyObject* list = PyList_New(0);
		if (list == 0)
		{
			//// ERROR
			encountered_errors = true;
		}

		for (unsigned int i = 0; i < names_len && !encountered_errors; ++i)
		{
			PyObject* tuple = Py_BuildValue("uu", names[i].friendly_name, names[i].symbolic_name);
			if (tuple == 0)
			{
				//// ERROR
				encountered_errors = true;
				break;
			}

			if (PyList_Append(list, tuple) == -1)
			{
				//// ERROR
				encountered_errors = true;
				break;
			}
		}

		if (!encountered_errors) result = list;
		else
		{
			if (list != 0) PyList_SetSlice(list, 0, PY_SSIZE_T_MAX, 0);
			Py_DECREF(list);
		}
		
		Holo_FreeCameraNames(names);
	}

	return result;
}

static PyModuleDef HoloCamModule = {
	.m_base    = PyModuleDef_HEAD_INIT,
	.m_name    = "holocam",
	.m_doc     = "Python wrapper for the Holo Cam virtual camera for windows 11",
	.m_size    = -1,
	.m_free    = py__Holo_Destroy,
	.m_methods = (PyMethodDef[]){
		{ "get_camera_names", py__Holo_GetCameraNames, METH_NOARGS, "get the unique and symbolic name of all cameras on the system as a list of tuples with unique & symbolic names" },
		{0}
	},
};

PyMODINIT_FUNC
PyInit_holocam(void)
{
	import_array();

	PyObject* module = 0;

	if (!Holo_Init((Holo_Settings){0}))
	{
		//// ERROR
		return 0;
	}
	else if (PyType_Ready(&py__HoloCamType) < 0)
	{
		//// ERROR
		return 0;
	}
	else if (PyType_Ready(&py__HoloCameraReaderType) < 0)
	{
		//// ERROR
		return 0;
	}
	else
	{
		module = PyModule_Create(&HoloCamModule);
		if (module == 0) return 0;

		if (PyModule_AddObjectRef(module, "HoloCam", (PyObject*)&py__HoloCamType) < 0)
		{
			Py_DECREF(module);
			return 0;
		}

		if (PyModule_AddObjectRef(module, "HoloCameraReader", (PyObject*)&py__HoloCameraReaderType) < 0)
		{
			Py_DECREF(module);
			return 0;
		}
	}

	return module;
}
