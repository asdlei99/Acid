#pragma once

#include "Axis.hpp"
#include "Devices/Joysticks.hpp"

namespace acid {
/**
 * @brief Axis input from a joystick input device.
 */
class ACID_EXPORT AxisJoystick : public Axis {
public:
	/**
	 * Creates a new axis joystick.
	 * @param port The joystick port.
	 * @param axis The axis on the joystick being checked.
	 */
	AxisJoystick(JoystickPort port, JoystickAxis axis);

	float GetAmount() const override;

	bool IsConnected() const;

	JoystickPort GetPort() const { return m_port; }
	void SetPort(JoystickPort port) { m_port = port; }

	JoystickAxis GetAxis() const { return m_axis; }
	void SetAxis(JoystickAxis axis) { m_axis = axis; }

private:
	JoystickPort m_port;
	JoystickAxis m_axis;
};
}
