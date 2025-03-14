import os
from openai import OpenAI

import json

client = OpenAI(
    # 若没有配置环境变量，请用百炼API Key将下行替换为：api_key="sk-xxx",
    # api_key=os.getenv("DASHSCOPE_API_KEY"), 
    api_key="sk-5554bc3278ef431e9099f5e05dbf6221",

    base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
)
completion = client.chat.completions.create(
    model="qwen-turbo", # 此处以qwen-plus为例，可按需更换模型名称。模型列表：https://help.aliyun.com/zh/model-studio/getting-started/models
    messages=[
        {'role': 'system', 'content': 'You are a helpful assistant.'},
        {'role': 'user', 'content': '你是谁啊？'}],
    )
    
json_string=completion.model_dump_json()
print(json_string)

# 将JSON字符串转换为Python字典
data = json.loads(json_string)

# 获取content内容
content = data['choices'][0]['message']['content']

print(content)