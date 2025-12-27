#include "mediafile.h"
Nalu Nalu::readNextNalu(std::ifstream& video_file) {
  Nalu nalu;

  // 1. 检查文件状态
  if (!video_file.is_open() || video_file.eof()) {
    return nalu;
  }

  // 2. 跳过当前的起始码 (Start Code)
  // H.264 起始码可能是 00 00 01 (3字节) 或 00 00 00 01 (4字节)
  uint8_t buf[4] = {0};
  video_file.read((char*)buf, 3);  // 先读 3 个字节
  size_t read_count = video_file.gcount();

  if (read_count < 3) return nalu;  // 文件太短

  // 判断是否是 00 00 01
  if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1) {
    // 这是一个 3 字节起始码，已经跳过完成了
  }
  // 判断是否是 00 00 00 ...
  else if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0) {
    // 读第 4 个字节看是不是 1
    video_file.read((char*)&buf[3], 1);
    if (video_file.gcount() == 1 && buf[3] == 1) {
      // 这是一个 4 字节起始码 (00 00 00 01)，跳过完成
    } else {
      // 不是标准起始码，可能文件损坏或指针错位
      // 简单处理：回退，尝试作为普通数据读取（或者报错）
      // 这里选择容错：回退读取的字节，让后续逻辑去处理
      video_file.seekg(-(int)video_file.gcount(), std::ios::cur);
    }
  } else {
    // 不是起始码，说明可能是在文件中间。回退指针，准备当做数据读。
    video_file.seekg(-3, std::ios::cur);
  }

  // 3. 逐字节读取数据，直到遇到下一个起始码或 EOF
  // 这种方法效率不是极致，但逻辑最清晰。优化可用 buffer 批量读。
  char byte;
  int zero_count = 0;  // 连续 0 的个数

  while (video_file.get(byte)) {
    nalu.data.push_back(static_cast<uint8_t>(byte));

    if (byte == 0x00) {
      zero_count++;
    } else if (byte == 0x01) {
      // 遇到 01，检查前面有几个 0
      if (zero_count >= 2) {
        // --- 发现下一个起始码！ ---
        // 现在的 nalu.data 尾部包含了 00 00 01 或 00 00 00 01
        // 我们需要把这些“下一个 NALU 的开头”从当前数据里删掉
        // 并把文件指针回退，留给下一次 readNextNalu 读取

        // 计算回退步数：
        // 如果是 00 00 01，回退 3 字节
        // 如果是 00 00 00 01，回退 4 字节
        // zero_count 是遇到 01 之前的 0 的个数

        int start_code_len = (zero_count >= 3) ? 4 : 3;

        // 从数据末尾移除起始码
        for (int i = 0; i < start_code_len; i++) {
          nalu.data.pop_back();
        }

        // 文件指针回退
        video_file.seekg(-start_code_len, std::ios::cur);

        // 标记读取成功
        nalu.is_valid = true;
        return nalu;
      }
      zero_count = 0;  // 01 但前面 0 不够，重置计数
    } else {
      zero_count = 0;  // 其他字符，重置计数
    }
  }

  // 4. 处理 EOF 情况
  // 如果读到了文件末尾，说明这是最后一个 NALU，它后面没有起始码了
  if (nalu.data.size() > 0) {
    nalu.is_valid = true;
  }
  return nalu;
}