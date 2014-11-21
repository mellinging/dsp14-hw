DSP HW1: Discrete Hidden Markov Model Implementation
====================================================

By David Lin (R02944024)

## Runtime Environment

- System: OS X Yosemite 10.10
- Processor: Intel Core i7
- Memory: 16 GB
- Compiler: clang (Apple LLVM version 6.0)

## Train

    ./train ITERATION INPUT_INIT_MODEL INPUT_SEQ OUTPUT_MODEL

- `ITERATION` 為正整數，表示training 要跑幾次 iterations
- `INPUT_INIT_MODEL` 是初始 model 檔案
- `INPUT_SEQ` 是訓練用的 observation data 檔案
- `OUTPUT_MODEL` 要輸出的 model 檔案

## Test

    ./test MODEL_LIST TEST_DATA RESULT

- `MODEL_LIST` 是一個文字檔，裡面列出所有的 models
- `TEST_DATA` 要跑測試用的檔案
- `RESULT` 輸出的 model 串列檔案

## Accuracy

此外，我還用 Ruby 語言寫了 `gen_acc`，用來比較 test result 與 reference result ，計算出答對的比率

    ./gen_acc RESULT ANSWER acc.txt

## Result

因為不同 iteration 次數所產生的 model 會影響 test 的 accuracy，
所以要觀察 accuracy 如何隨 #(iterations) 而變化。

我把 `testing_data-1` 的 accuracy 依照不同的 #(iterations) 來記錄，
用圖表來顯示 accuracy 的變化。

跟助教給的參考圖表蠻類似，會在 #(iterations) = 10 時候 accuracy 下降，
爾後就回升，然後在 #(iterations) = 750 時候趨於穩定。

| #(Iter) | Accuracy |
| ------- | -------: |
|      5  |  0.6484  |
|     10  |  0.5408  |
|     20  |  0.7912  |
|     40  |  0.8212  |
|     80  |  0.8072  |
|    100  |  0.8100  |
|    160  |  0.8588  |
|    200  |  0.8528  |
|    320  |  0.8504  |
|    400  |  0.8540  |
|    500  |  0.8560  |
|    640  |  0.8636  |
|    750  |  0.8676  |
|   1000  |  0.8696  |
|   1280  |  0.8700  |
|   1500  |  0.8700  |
|   2560  |  0.8680  |

![Result](result.png)
