import os
from openai import OpenAI

client = OpenAI(
    # 若没有配置环境变量，请用百炼API Key将下行替换为：api_key="sk-xxx",
    # api_key=os.getenv("DASHSCOPE_API_KEY"),
    api_key="sk-5554bc3278ef431e9099f5e05dbf6221",
    base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
)

completion = client.chat.completions.create(
    model="qwen-omni-turbo",
    messages=[{"role": "user", "content": "今天的天气"}],
    # 设置输出数据的模态，当前仅支持["text"]
    modalities=["text"],
    # stream 必须设置为 True，否则会报错
    stream=True,
    stream_options={"include_usage": True},
)

for chunk in completion:
    if chunk.choices:
        print(chunk.choices[0].delta)
    else:
        print(chunk.usage)