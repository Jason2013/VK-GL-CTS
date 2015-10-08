#ifndef _VKTDYNAMICSTATEBASECLASS_HPP
#define _VKTDYNAMICSTATEBASECLASS_HPP
/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 The Khronos Group Inc.
 * Copyright (c) 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * The Materials are Confidential Information as defined by the
 * Khronos Membership Agreement until designated non-confidential by Khronos,
 * at which point this condition clause shall be removed.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 *//*!
 * \file
 * \brief Dynamic State Tests - Base Class
 *//*--------------------------------------------------------------------*/

#include "vktTestCase.hpp"

#include "tcuTestLog.hpp"
#include "tcuResource.hpp"
#include "tcuImageCompare.hpp"
#include "tcuCommandLine.hpp"

#include "vkRefUtil.hpp"
#include "vkImageUtil.hpp"

#include "vktDynamicStateImageObjectUtil.hpp"
#include "vktDynamicStateBufferObjectUtil.hpp"
#include "vktDynamicStateCreateInfoUtil.hpp"
#include "vkPrograms.hpp"

namespace vkt
{
namespace DynamicState
{

inline tcu::Vec4 vec4Red (void)
{
	return tcu::Vec4(1.0f, 0.0f, 0.0f, 1.0f);
}

inline tcu::Vec4 vec4Green (void)
{
	return tcu::Vec4(0.0f, 1.0f, 0.0f, 1.0f);
}

inline tcu::Vec4 vec4Blue (void)
{
	return tcu::Vec4(0.0f, 0.0f, 1.0f, 1.0f);
}

struct Vec4RGBA
{
	Vec4RGBA(tcu::Vec4 p, tcu::Vec4 c)
	: position(p)
	, color(c)
	{}
	tcu::Vec4 position;
	tcu::Vec4 color;
};

vk::Move<vk::VkShader> createShader(const vk::DeviceInterface &vk, const vk::VkDevice device, 
									const vk::VkShaderModule module, const char* name, vk::VkShaderStage stage);
	
class DynamicStateBaseClass : public TestInstance
{
public:
	DynamicStateBaseClass(Context &context, const char* vertexShaderName, const char* fragmentShaderName)
		: TestInstance				(context)
		, m_colorAttachmentFormat   (vk::VK_FORMAT_R8G8B8A8_UNORM)
		, m_topology				(vk::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP)
		, m_vertexShaderName		(vertexShaderName)
		, m_fragmentShaderName		(fragmentShaderName)
		, m_vk						(context.getDeviceInterface())
	{
	}

protected:

	enum 
	{
		WIDTH       = 128,
		HEIGHT      = 128
	};

	vk::VkFormat									m_colorAttachmentFormat;

	vk::VkPrimitiveTopology							m_topology;

	const vk::DeviceInterface&						m_vk;

	vk::Move<vk::VkPipeline>						m_pipeline;
	vk::Move<vk::VkPipelineLayout>					m_pipelineLayout;

	de::SharedPtr<Image>							m_colorTargetImage;
	vk::Move<vk::VkImageView>						m_colorTargetView;

	PipelineCreateInfo::VertexInputState			m_vertexInputState;
	de::SharedPtr<Buffer>							m_vertexBuffer;

	vk::Move<vk::VkCmdPool>							m_cmdPool;
	vk::Move<vk::VkCmdBuffer>						m_cmdBuffer;

	vk::Move<vk::VkFramebuffer>						m_framebuffer;
	vk::Move<vk::VkRenderPass>						m_renderPass;

	const std::string								m_vertexShaderName;
	const std::string								m_fragmentShaderName;

	std::vector<Vec4RGBA>							m_data;

	void initialize (void)
	{
		tcu::TestLog &log = m_context.getTestContext().getLog();
		const vk::VkDevice device = m_context.getDevice();
		const deUint32 queueFamilyIndex = m_context.getUniversalQueueFamilyIndex();

		const PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		m_pipelineLayout = vk::createPipelineLayout(m_vk, device, &pipelineLayoutCreateInfo);

		const vk::VkExtent3D targetImageExtent = { WIDTH, HEIGHT, 1 };
		const ImageCreateInfo targetImageCreateInfo(vk::VK_IMAGE_TYPE_2D, m_colorAttachmentFormat, targetImageExtent, 1, 1, 1, 
			vk::VK_IMAGE_TILING_OPTIMAL, vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | vk::VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT);

		m_colorTargetImage = Image::CreateAndAlloc(m_vk, device, targetImageCreateInfo, m_context.getDefaultAllocator());

		const ImageViewCreateInfo colorTargetViewInfo(m_colorTargetImage->object(), vk::VK_IMAGE_VIEW_TYPE_2D, m_colorAttachmentFormat);
		m_colorTargetView = vk::createImageView(m_vk, device, &colorTargetViewInfo);

		RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.addAttachment(AttachmentDescription(
			m_colorAttachmentFormat,
			1,
			vk::VK_ATTACHMENT_LOAD_OP_LOAD,
			vk::VK_ATTACHMENT_STORE_OP_STORE,
			vk::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			vk::VK_ATTACHMENT_STORE_OP_STORE,
			vk::VK_IMAGE_LAYOUT_GENERAL,
			vk::VK_IMAGE_LAYOUT_GENERAL
			)
			);

		const vk::VkAttachmentReference colorAttachmentReference =
		{
			0,
			vk::VK_IMAGE_LAYOUT_GENERAL
		};

		renderPassCreateInfo.addSubpass(SubpassDescription(
			vk::VK_PIPELINE_BIND_POINT_GRAPHICS,
			0,
			0,
			DE_NULL,
			1,
			&colorAttachmentReference,
			DE_NULL,
			AttachmentReference(),
			0,
			DE_NULL
			)
			);

		m_renderPass = vk::createRenderPass(m_vk, device, &renderPassCreateInfo);

		std::vector<vk::VkImageView> colorAttachments(1);
		colorAttachments[0] = *m_colorTargetView;

		const FramebufferCreateInfo framebufferCreateInfo(*m_renderPass, colorAttachments, WIDTH, HEIGHT, 1);

		m_framebuffer = vk::createFramebuffer(m_vk, device, &framebufferCreateInfo);

		const vk::VkVertexInputBindingDescription vertexInputBindingDescription =
		{
			0,
			sizeof(tcu::Vec4) * 2,
			vk::VK_VERTEX_INPUT_STEP_RATE_VERTEX,
		};

		const vk::VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] =
		{
			{
				0u,
				0u,
				vk::VK_FORMAT_R32G32B32A32_SFLOAT,
				0u
			},
			{
				1u,
				0u,
				vk::VK_FORMAT_R32G32B32A32_SFLOAT,
				(deUint32)(sizeof(float)* 4),
			}
		};

		m_vertexInputState = PipelineCreateInfo::VertexInputState(
			1,
			&vertexInputBindingDescription,
			2,
			vertexInputAttributeDescriptions);

		const vk::VkDeviceSize dataSize = m_data.size() * sizeof(Vec4RGBA);
		m_vertexBuffer = Buffer::CreateAndAlloc(m_vk, device, BufferCreateInfo(dataSize,
			vk::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT), m_context.getDefaultAllocator(), vk::MemoryRequirement::HostVisible);

		unsigned char *ptr = reinterpret_cast<unsigned char *>(m_vertexBuffer->getBoundMemory().getHostPtr());
		deMemcpy(ptr, &m_data[0], dataSize);

		vk::flushMappedMemoryRange(m_vk, device, 
			m_vertexBuffer->getBoundMemory().getMemory(),
			m_vertexBuffer->getBoundMemory().getOffset(),
			dataSize);

		const CmdPoolCreateInfo cmdPoolCreateInfo(queueFamilyIndex);
		m_cmdPool = vk::createCommandPool(m_vk, device, &cmdPoolCreateInfo);

		const CmdBufferCreateInfo cmdBufCreateInfo(*m_cmdPool, vk::VK_CMD_BUFFER_LEVEL_PRIMARY, 0);
		m_cmdBuffer = vk::createCommandBuffer(m_vk, device, &cmdBufCreateInfo);

		initPipeline(device);
	}

	virtual void initPipeline (const vk::VkDevice device)
	{
		const vk::Unique<vk::VkShader> vs(createShader(m_vk, device,
			*createShaderModule(m_vk, device, m_context.getBinaryCollection().get(m_vertexShaderName), 0),
			"main", vk::VK_SHADER_STAGE_VERTEX));

		const vk::Unique<vk::VkShader> fs(createShader(m_vk, device,
			*createShaderModule(m_vk, device, m_context.getBinaryCollection().get(m_fragmentShaderName), 0),
			"main", vk::VK_SHADER_STAGE_FRAGMENT));

		const PipelineCreateInfo::ColorBlendState::Attachment vkCbAttachmentState;

		PipelineCreateInfo pipelineCreateInfo(*m_pipelineLayout, *m_renderPass, 0, 0);
		pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*vs, vk::VK_SHADER_STAGE_VERTEX));
		pipelineCreateInfo.addShader(PipelineCreateInfo::PipelineShaderStage(*fs, vk::VK_SHADER_STAGE_FRAGMENT));
		pipelineCreateInfo.addState(PipelineCreateInfo::VertexInputState(m_vertexInputState));
		pipelineCreateInfo.addState(PipelineCreateInfo::InputAssemblerState(m_topology));
		pipelineCreateInfo.addState(PipelineCreateInfo::ColorBlendState(1, &vkCbAttachmentState));
		pipelineCreateInfo.addState(PipelineCreateInfo::ViewportState(1));
		pipelineCreateInfo.addState(PipelineCreateInfo::DepthStencilState());
		pipelineCreateInfo.addState(PipelineCreateInfo::RasterizerState());
		pipelineCreateInfo.addState(PipelineCreateInfo::MultiSampleState());
		pipelineCreateInfo.addState(PipelineCreateInfo::DynamicState());

		m_pipeline = vk::createGraphicsPipeline(m_vk, device, DE_NULL, &pipelineCreateInfo);
	}

	virtual tcu::TestStatus iterate (void)
	{
		TCU_FAIL("Implement iterate() method!");
	}

	void beginRenderPass (void)
	{
		const vk::VkClearColorValue clearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		beginRenderPassWithClearColor(clearColor);
	}

	void beginRenderPassWithClearColor (const vk::VkClearColorValue &clearColor)
	{
		const CmdBufferBeginInfo beginInfo;
		m_vk.beginCommandBuffer(*m_cmdBuffer, &beginInfo);

		initialTransitionColor2DImage(m_vk, *m_cmdBuffer, m_colorTargetImage->object(), vk::VK_IMAGE_LAYOUT_GENERAL);

		const ImageSubresourceRange subresourceRange(vk::VK_IMAGE_ASPECT_COLOR_BIT);
		m_vk.cmdClearColorImage(*m_cmdBuffer, m_colorTargetImage->object(),
			vk::VK_IMAGE_LAYOUT_GENERAL, &clearColor, 1, &subresourceRange);

		const vk::VkRect2D renderArea = { { 0, 0 }, { WIDTH, HEIGHT } };
		const RenderPassBeginInfo renderPassBegin(*m_renderPass, *m_framebuffer, renderArea);

		m_vk.cmdBeginRenderPass(*m_cmdBuffer, &renderPassBegin, vk::VK_RENDER_PASS_CONTENTS_INLINE);
	}

	void setDynamicViewportState (const deUint32 width, const deUint32 height)
	{
		vk::VkViewport viewport;
		viewport.originX = 0;
		viewport.originY = 0;
		viewport.width = static_cast<float>(width);
		viewport.height = static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		m_vk.cmdSetViewport(*m_cmdBuffer, 1, &viewport);

		vk::VkRect2D scissor;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = width;
		scissor.extent.height = height;
		m_vk.cmdSetScissor(*m_cmdBuffer, 1, &scissor);
	}

	void setDynamicViewportState (deUint32 viewportCount, const vk::VkViewport* pViewports, const vk::VkRect2D* pScissors)
	{
		m_vk.cmdSetViewport(*m_cmdBuffer, viewportCount, pViewports);
		m_vk.cmdSetScissor(*m_cmdBuffer, viewportCount, pScissors);
	}

	void setDynamicRasterState (const float lineWidth = 1.0f, 
								const float depthBias = 0.0f, 
								const float depthBiasClamp = 0.0f, 
								const float slopeScaledDepthBias = 0.0f)
	{
		m_vk.cmdSetLineWidth(*m_cmdBuffer, lineWidth);
		m_vk.cmdSetDepthBias(*m_cmdBuffer, depthBias, depthBiasClamp, slopeScaledDepthBias);
	}

	void setDynamicBlendState (const float const1 = 0.0f, const float const2 = 0.0f, 
							   const float const3 = 0.0f, const float const4 = 0.0f)
	{
		float blendConstants[4] = { const1, const2, const3, const4 };
		m_vk.cmdSetBlendConstants(*m_cmdBuffer, blendConstants);
	}

	void setDynamicDepthStencilState (const float minDepthBounds = -1.0f, 
									  const float maxDepthBounds = 1.0f,
									  const deUint32 stencilFrontCompareMask = 0xffffffffu, 
									  const deUint32 stencilFrontWriteMask = 0xffffffffu,
									  const deUint32 stencilFrontReference = 0, 
									  const deUint32 stencilBackCompareMask = 0xffffffffu, 
									  const deUint32 stencilBackWriteMask = 0xffffffffu, 
									  const deUint32 stencilBackReference = 0)
	{
		m_vk.cmdSetDepthBounds(*m_cmdBuffer, minDepthBounds, maxDepthBounds);
		m_vk.cmdSetStencilCompareMask(*m_cmdBuffer, vk::VK_STENCIL_FACE_FRONT_BIT, stencilFrontCompareMask);
		m_vk.cmdSetStencilWriteMask(*m_cmdBuffer, vk::VK_STENCIL_FACE_FRONT_BIT, stencilFrontWriteMask);
		m_vk.cmdSetStencilReference(*m_cmdBuffer, vk::VK_STENCIL_FACE_FRONT_BIT, stencilFrontReference);
		m_vk.cmdSetStencilCompareMask(*m_cmdBuffer, vk::VK_STENCIL_FACE_BACK_BIT, stencilBackCompareMask);
		m_vk.cmdSetStencilWriteMask(*m_cmdBuffer, vk::VK_STENCIL_FACE_BACK_BIT, stencilBackWriteMask);
		m_vk.cmdSetStencilReference(*m_cmdBuffer, vk::VK_STENCIL_FACE_BACK_BIT, stencilBackReference);
	}
};

} //DynamicState
} //vkt

#endif // _VKTDYNAMICSTATEBASECLASS_HPP
