import nncase_runtime as nn
import ulab.numpy as np

# init kpu and load kmodel
kpu = nn.kpu()
kpu.load_kmodel("/sharefs/nfs/mbv2/test.kmodel")

# dump model input and output info
print("inputs info:")
for i in range(kpu.inputs_size()):
    print(kpu.inputs_desc(i))

print("outputs info:")
for i in range(kpu.outputs_size()):
    print(kpu.outputs_desc(i))


# set input tensor

with open('/sharefs/nfs/mbv2/input_0_0.bin', 'rb') as f:
    data = f.read()

input_data = np.frombuffer(data, dtype=np.float)
input_data = input_data.reshape((1,3,224,224))

# data = np.randm.rand([1,3,320,320],dtype=np.uint8)
kpu.set_input_tensor(0, nn.from_numpy(input_data))


# run kmodel
kpu.run()


# get output
result = kpu.get_output_tensor(0)
result = result.to_numpy()
print(result.shape,result.dtype)

