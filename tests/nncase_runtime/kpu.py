import nncase_runtime as nn
import ulab.numpy as np
import gc

# We will explain how to use nncase_runtime in this test script for `KPU`,
# including model reading, printing input and output information of the model,
# configuring input data, and how to obtain output.

# init kpu and load kmodel
kpu = nn.kpu()
kpu.load_kmodel("/sdcard/app/tests/nncase_runtime/face_detection/face_detection_320.kmodel")

# dump model input and output info
print("inputs info:")
for i in range(kpu.inputs_size()):
    print(kpu.inputs_desc(i))

print("outputs info:")
for i in range(kpu.outputs_size()):
    print(kpu.outputs_desc(i))

# set input tensor
with open('/sdcard/app/tests/nncase_runtime/face_detection/face_detection_ai2d_output.bin', 'rb') as f:
    data = f.read()

input_data = np.frombuffer(data, dtype=np.uint8)
input_data = input_data.reshape((1,3,320,320))
kpu.set_input_tensor(0, nn.from_numpy(input_data))

# run kmodel
kpu.run()

# get output
for i in range(kpu.outputs_size()):
    data = kpu.get_output_tensor(i).to_numpy()
    print("result: ", i, data.flatten()[-5:])
    print(data.shape, data.dtype)

del kpu
gc.collect()
nn.shrink_memory_pool()
