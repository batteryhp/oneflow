#include "oneflow/core/job/job_set.pb.h"
#include "oneflow/core/kernel/model_init_kernel.h"

namespace oneflow {

template<DeviceType device_type, typename T>
void ModelInitKernel<device_type, T>::VirtualKernelInit(const ParallelContext* parallel_ctx) {
  std::tie(part_id_, part_num_) = GetPartIdAndPartNumFromParallelCtx(parallel_ctx);
  random_seed_gen_.reset(
      new std::mt19937(this->op_conf().model_init_conf().original_variable_conf().random_seed()));
}

template<DeviceType device_type, typename T>
void ModelInitKernel<device_type, T>::Forward(
    const KernelCtx& ctx, std::function<Blob*(const std::string&)> BnInOp2Blob) const {
  Blob* out_blob = BnInOp2Blob("out");
  const ModelInitOpConf& op_conf = this->op_conf().model_init_conf();
  const std::string& snapshot_path = Global<const IOConf>::Get()->model_load_snapshot_path();
  if (snapshot_path == "") {
    KernelUtil<device_type, T>::InitializeWithProperConf(
        ctx.device_ctx,
        this->GetInitializerFromPbMessage(op_conf.original_variable_conf(), "initializer"),
        (*random_seed_gen_)(), out_blob);
  } else {
    std::string load_path = JoinPath(snapshot_path, op_conf.variable_op_name());
    std::string model_name = op_conf.original_variable_conf().model_name();
    KernelUtil<device_type, T>::InitializeWithDir(ctx.device_ctx, part_id_, part_num_, load_path,
                                                  out_blob, model_name, out_blob->shape().At(0),
                                                  out_blob->shape().Count(1));
  }
}

#define REGISTER_MODEL_INIT_KERNEL(dev, dtype)                                    \
  REGISTER_KERNEL_WITH_DEVICE_AND_DTYPE(OperatorConf::kModelInitConf, dev, dtype, \
                                        ModelInitKernel<dev, dtype>)

REGISTER_MODEL_INIT_KERNEL(DeviceType::kCPU, float);
REGISTER_MODEL_INIT_KERNEL(DeviceType::kCPU, double);
REGISTER_MODEL_INIT_KERNEL(DeviceType::kGPU, float);
REGISTER_MODEL_INIT_KERNEL(DeviceType::kGPU, double);

}  // namespace oneflow
