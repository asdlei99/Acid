#pragma once

#include <string>
#include <vector>
#include "../Objects/Component.hpp"
#include "../Models/Model.hpp"

namespace Flounder
{
	class F_EXPORT Mesh :
		public Component
	{
	private:
		Model *m_model;

	public:
		Mesh(Model *model = nullptr);

		~Mesh();

		void Update() override;

		std::string GetName() const override { return "Mesh"; };

		Model *GetModel() const { return m_model; }

		void SetModel(Model *model) { m_model = model; }
	private:
		static Model *DecodeModelType(const std::string &filename);
	};
}