from tflite_runtime.interpreter import Interpreter
import sounddevice as sd
import numpy as np
import librosa
import joblib
from scipy.io.wavfile import write
import requests
import os

param1 = {'trigger' : 1}
param2 = {'trigger' : 0}


interpreter = Interpreter('model_25m_2_1.tflite')
interpreter.allocate_tensors()
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

flag = 0

trig = [0,0]

for i in range(100):
    fs = 44100
    seconds = 1
    myrecording = sd.rec(int(seconds * fs), samplerate=fs, channels=1)
    sd.wait() 
    write('output.wav', fs, myrecording)
    print('Recording saved')    

    path = 'output.wav'
    data, sr = librosa.load(path,sr=None,mono=True,offset=0.0, duration = None)
    count = int(len(data)/1103)
    mfcc_list = []
    start = 0
    end = 1103
    for j in range(count):
        test = data[start:end]
        mfcc = librosa.feature.mfcc(y=test, sr=sr, n_mfcc=12)
        mfcc = np.array(mfcc)
        mfcc = mfcc.flatten()
        mfcc_list.append(mfcc)
        start = start + 1103
        end = end + 1103
        
    scaler = joblib.load('updated_scaler_25.save')
    mfcc_list = np.array(mfcc_list)
    scaler.fit_transform(mfcc_list)
    y_pred = []
    
    for t in mfcc_list:
        t = np.reshape(t,(1,36))
        t = t.astype('float32')
        interpreter.set_tensor(input_details[0]['index'], t )
        interpreter.invoke()
        output_data = interpreter.get_tensor(output_details[0]['index'])
        y_pred.append(output_data)
    
    ID = 0
    if np.mean(y_pred) > 0.5 :
        ID = 1
    print(str(np.mean(y_pred))+' : '+str(ID))
    

    if ((i+1)%2) == 0:
        trig[1] = ID
    else:
        trig[0] = ID
        
    a = os.popen('hostname -I').read()
    
    if np.sum(trig) == 2 and len(a)>1:
        print('Triggering request sent to Master D1 mini');
        flag = 1
        response1 = requests.post('http://192.168.4.15:88/RPI',data=param1)
        print(response1.text)
    elif np.sum(trig) == 0 and len(a)>1:
        print('Detriggering request sent to Master D1 mini');
        flag = 0
        response2 = requests.post('http://192.168.4.15:88/RPI',data=param2)
        print(response2.text)
    else:
        print('Not connected to wifi')
