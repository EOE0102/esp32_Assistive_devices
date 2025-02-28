#include "Audio_Record.h"



// SD卡配置 (XIAO ESP32S3专用)
#define SD_CS_PIN 21       // SD卡片选引脚
#define WAV_FILE_NAME "/recording.wav"
#define SAMPLE_RATE 16000  // 必须与I2S配置一致
#define CHANNELS 1         // 单声道
#define WAV_HEADER_SIZE 44

Audio_Record::Audio_Record(uint8_t PIN_I2S_BCLK, uint8_t PIN_I2S_LRC, uint8_t PIN_I2S_DIN)
{
  // 构造函数中初始化成员变量和分配内存
  wavData = nullptr;
  i2s = nullptr;
  i2s = new MY_I2S(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DIN);

  // wavData = new char*[wavDataSize/dividedWavDataSize];
  // for (int i = 0; i < wavDataSize/dividedWavDataSize; ++i) wavData[i] = new char[dividedWavDataSize];
  // i2s = new I2S(micType);
}

Audio_Record::~Audio_Record()
{
  for (int i = 0; i < wavDataSize / dividedWavDataSize; ++i)
    delete[] wavData[i];
  delete[] wavData;
  delete i2s;
}

void Audio_Record::init()
{
  wavData = new char *[1];
  for (int i = 0; i < 1; ++i)
    wavData[i] = new char[1280];
}

void Audio_Record::clear()
{
  i2s->clear();
}

void Audio_Record::CreateWavHeader(byte *header, int waveDataSize)
{
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSizeMinus8 = waveDataSize + 44 - 8;
  header[4] = (byte)(fileSizeMinus8 & 0xFF);
  header[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  header[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  header[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10; // 线性PCM
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01; // 线性PCM
  header[21] = 0x00;
  header[22] = 0x01; // 单声道
  header[23] = 0x00;
  // 修正采样率设置
  uint32_t sampleRate = SAMPLE_RATE;

  header[24] = (byte)(sampleRate & 0xFF);
  header[25] = (byte)((sampleRate >> 8) & 0xFF);
  header[26] = (byte)((sampleRate >> 16) & 0xFF);
  header[27] = (byte)((sampleRate >> 24) & 0xFF);

  // 修正字节率计算
  uint32_t byteRate = SAMPLE_RATE * 2 * CHANNELS; // 16bit = 2 bytes
  header[28] = (byte)(byteRate & 0xFF);
  header[29] = (byte)((byteRate >> 8) & 0xFF);
  header[30] = (byte)((byteRate >> 16) & 0xFF);
  header[31] = (byte)((byteRate >> 24) & 0xFF);

  header[32] = 0x02; // 16位单声道 BlockAlign
  header[33] = 0x00;
  header[34] = 0x10; // 16位 BitsPerSample (16位)
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(waveDataSize & 0xFF);
  header[41] = (byte)((waveDataSize >> 8) & 0xFF);
  header[42] = (byte)((waveDataSize >> 16) & 0xFF);
  header[43] = (byte)((waveDataSize >> 24) & 0xFF);
}




void Audio_Record::Record()
{
  i2s->Read(i2sBuffer, i2sBufferSize);
  for (int i = 0; i < i2sBufferSize / 8; ++i)
  {
    wavData[0][2 * i] = i2sBuffer[8 * i + 2];
    wavData[0][2 * i + 1] = i2sBuffer[8 * i + 3];
  }
}





void Audio_Record::SaveFile()
{
  // 初始化SD卡
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Mount Failed");
    return;
  }

  // 删除旧文件
  if (SD.exists(WAV_FILE_NAME)) {
    SD.remove(WAV_FILE_NAME);
  }

  // 创建WAV文件并写入初始头
  File wavFile = SD.open(WAV_FILE_NAME, FILE_WRITE);
  if (!wavFile) {
    Serial.println("Create File Failed");
    return;
  }

  // 生成初始WAV头（数据长度置0）
  byte wavHeader[44];
  CreateWavHeader(wavHeader, 0);
  wavFile.write(wavHeader, sizeof(wavHeader));

  uint32_t totalDataSize = 0;
  bool recording = true;

  while (recording) {
    // 采集音频数据
    Record();  // 调用现有Record方法填充wavData
    
    // 写入SD卡（每次写入1280字节）
    size_t written = wavFile.write((const uint8_t*)wavData[0], 1280);
    
    if (written != 1280) {
      Serial.println("Write Error");
      break;
    }
    
    totalDataSize += written;

    // 添加停止条件（示例：按按键停止）
    if (recording) {
      recording = false;
    }
  }

  // 回写更新WAV头
  wavFile.seek(0);
  CreateWavHeader(wavHeader, totalDataSize);
  wavFile.write(wavHeader, sizeof(wavHeader));
  wavFile.close();

  Serial.printf("Recording Saved: %u bytes\n", totalDataSize);

}




String Audio_Record::parseJSON(const char *jsonResponse)
{
  DynamicJsonDocument doc(1024);

  // 解析JSON响应
  DeserializationError error = deserializeJson(doc, jsonResponse);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return String("");
  }

  // 提取并返回"question"
  const char *question = doc["result"][0];
  return String(question);
}

float Audio_Record::calculateRMS(uint8_t *buffer, int bufferSize)
{
  float sum = 0;
  int16_t sample;

  // 每次处理两个字节，16位
  for (int i = 0; i < bufferSize; i += 2)
  {
    // 从缓冲区中读取16位样本，注意字节顺序
    sample = (buffer[i + 1] << 8) | buffer[i];

    // 计算平方和
    sum += sample * sample;
  }

  // 计算平均值
  sum /= (bufferSize / 2);

  // 返回RMS值
  return sqrt(sum);
}


