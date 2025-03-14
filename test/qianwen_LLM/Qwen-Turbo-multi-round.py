import os
from openai import OpenAI


def get_response(messages):
    client = OpenAI(
        # 若没有配置环境变量，请用百炼API Key将下行替换为：api_key="sk-xxx",
        # api_key=os.getenv("DASHSCOPE_API_KEY"),
        api_key="sk-5554bc3278ef431e9099f5e05dbf6221",
        base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
    )
    # 模型列表：https://help.aliyun.com/zh/model-studio/getting-started/models
    completion = client.chat.completions.create(model="qwen-plus", messages=messages)
    return completion

# 初始化一个 messages 数组
messages = [
    {
        "role": "system",
        "content": """你是一个辅助残疾人对话的大语言模型，你的名字是小曦。你的主要工作是提供情绪价值，为用户提供各类对话服务，让用户开心。
        回话的部分不要带有任何表情包。
        如果用户表示需要结束对话的情况，你要说，好的，期待和你的下一次对话""",
    }
]
assistant_output = "你好，我是小曦"
print(f"模型输出：{assistant_output}\n")
while "期待和你的下一次对话" not in assistant_output:
    user_input = input("请输入：")
    # 将用户问题信息添加到messages列表中
    messages.append({"role": "user", "content": user_input})
    assistant_output = get_response(messages).choices[0].message.content
    # 将大模型的回复信息添加到messages列表中
    messages.append({"role": "assistant", "content": assistant_output})

    for i, item in enumerate(messages):
        print(f"第 {i+1} 个字典的内容:")
        # 遍历字典中的每个键值对
        for key, value in item.items():
            print(f"  {key}: {value}")


    print(f"模型输出：{assistant_output}")
    print("\n")