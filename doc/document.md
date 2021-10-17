• how you handle comments;

在遇到`/*`符号的时候进入COMMENT模式，comment_level初始化为1，遇到`/*`就增加将comment_level加一，遇到`*/`就将comment_level减1，等于0的时候总结注释跳出COMMENT模式。

• how you handle strings;

检测到`"`的时候开始，再次遇到`"`的时候结束。中间根据tiger语言对字符串的识别规则增加响应的识别处理

• error handling;

遇到异常无法匹配,调用`errormsg->Error()`函数，将错误位置的下标报出。

