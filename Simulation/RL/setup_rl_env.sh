#!/bin/bash

echo "🔧 Đang tạo môi trường ảo tên 'venv_rl'..."
python3 -m venv ~/venv_rl

echo "✅ Môi trường ảo đã tạo tại ~/venv_rl"
echo "🧠 Đang kích hoạt môi trường và cài các gói cần thiết..."

source ~/venv_rl/bin/activate
pip install --upgrade pip
pip install ipykernel jupyter notebook

echo "✅ Cài đặt xong ipykernel & jupyter"
echo "🔁 Đăng ký kernel 'rl_env' cho Jupyter..."

python -m ipykernel install --user --name=rl_env --display-name "Python (RL Env)"

echo "✅ Kernel 'Python (RL Env)' đã được tạo."
echo "🚀 Bạn có thể chạy Jupyter Notebook bằng lệnh:"

echo "source ~/venv_rl/bin/activate && jupyter notebook"

source ~/venv_rl/bin/activate && cd ~/VHT2024/CapstoneProject/Simulation/RL && jupyter notebook

