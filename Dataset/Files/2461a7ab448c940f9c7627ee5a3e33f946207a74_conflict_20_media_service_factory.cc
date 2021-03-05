// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_service_factory.h"

#include <memory>

#include "base/logging.h"
<<<<<<< HEAD
#include "media/media_buildflags.h"
=======
#include "build/build_config.h"
>>>>>>> e541442e968aa97bea018fc771253d209fae984a
#include "media/mojo/buildflags.h"
#include "media/mojo/services/gpu_mojo_media_client.h"
#include "media/mojo/services/media_service.h"
#include "media/mojo/services/test_mojo_media_client.h"

#if defined(OS_ANDROID)
#include "media/mojo/services/android_mojo_media_client.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_MEDIA_FOUNDATION_CDM)
#include "media/base/win/mf_cdm_trace.h"
#include "media/mojo/services/mfmediaengine_mojo_media_client.h"
#endif

namespace media {

std::unique_ptr<MediaService> CreateMediaService(
    mojo::PendingReceiver<mojom::MediaService> receiver) {
#if defined(OS_ANDROID)
  return std::make_unique<MediaService>(
      std::make_unique<AndroidMojoMediaClient>(), std::move(receiver));
#elif BUILDFLAG(ENABLE_MEDIA_FOUNDATION_CDM)
  MFCDM_TRACE_MESSAGE0("new MediaService with MFMediaEngineMojoMediaClient.");
  return std::unique_ptr<MediaService>(
      new MediaService(std::make_unique<media::MFMediaEngineMojoMediaClient>(),
                       std::move(receiver)));
#else
  NOTREACHED() << "No MediaService implementation available.";
  return nullptr;
#endif
}

std::unique_ptr<MediaService> CreateGpuMediaService(
    mojo::PendingReceiver<mojom::MediaService> receiver,
    const gpu::GpuPreferences& gpu_preferences,
    const gpu::GpuDriverBugWorkarounds& gpu_workarounds,
    const gpu::GpuFeatureInfo& gpu_feature_info,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    base::WeakPtr<MediaGpuChannelManager> media_gpu_channel_manager,
    gpu::GpuMemoryBufferFactory* gpu_memory_buffer_factory,
    AndroidOverlayMojoFactoryCB android_overlay_factory_cb,
    CdmProxyFactoryCB cdm_proxy_factory_cb) {
  return std::make_unique<MediaService>(
      std::make_unique<GpuMojoMediaClient>(
          gpu_preferences, gpu_workarounds, gpu_feature_info, task_runner,
          media_gpu_channel_manager, gpu_memory_buffer_factory,
          std::move(android_overlay_factory_cb),
          std::move(cdm_proxy_factory_cb)),
      std::move(receiver));
}

std::unique_ptr<MediaService> CreateMediaServiceForTesting(
    mojo::PendingReceiver<mojom::MediaService> receiver) {
  return std::make_unique<MediaService>(std::make_unique<TestMojoMediaClient>(),
                                        std::move(receiver));
}

}  // namespace media
