import platform

import torch


def main():
    print(f"Python platform: {platform.platform()}")
    print(f"PyTorch version: {torch.__version__}")
    print(f"PyTorch CUDA runtime: {torch.version.cuda}")
    print(f"CUDA available: {torch.cuda.is_available()}")

    if not torch.cuda.is_available():
        raise SystemExit(
            "CUDA is not available. Check the NVIDIA driver and reinstall the CUDA-enabled PyTorch wheel."
        )

    print(f"GPU count: {torch.cuda.device_count()}")
    for index in range(torch.cuda.device_count()):
        properties = torch.cuda.get_device_properties(index)
        memory_gb = properties.total_memory / (1024 ** 3)
        print(f"GPU {index}: {properties.name}")
        print(f"  Compute capability: {properties.major}.{properties.minor}")
        print(f"  Total VRAM: {memory_gb:.2f} GB")

    tensor = torch.randn((2048, 2048), device="cuda")
    result = tensor @ tensor
    torch.cuda.synchronize()
    print(f"CUDA calculation succeeded: shape={tuple(result.shape)}")


if __name__ == "__main__":
    main()
