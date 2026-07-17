import esphome.codegen as cg
from esphome.components import output, socket
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT

CODEOWNERS = ["@kaboom748"]
DEPENDENCIES = ["network"]
AUTO_LOAD = ["socket"]

virtual_output_ns = cg.esphome_ns.namespace("virtual_output")
VirtualOutput = virtual_output_ns.class_(
    "VirtualOutput", output.FloatOutput, cg.Component
)

CONFIG_SCHEMA = cv.All(
    # inverted / min_power / max_power / zero_means_zero come from the base schema,
    # and FloatOutput::set_level() applies them before write_state() is called.
    # So the browser receives the duty cycle the pin would carry, for free.
    output.FLOAT_OUTPUT_SCHEMA.extend(
        {
            cv.Required(CONF_ID): cv.declare_id(VirtualOutput),
            cv.Optional(CONF_PORT, default=8082): cv.port,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    # The socket component sizes lwIP's tables from these declarations. One
    # listening socket, plus two connections: the live stream and a transient
    # GET / that may arrive while it runs.
    socket.consume_sockets(2, "virtual_output"),
    socket.consume_sockets(1, "virtual_output", socket.SocketType.TCP_LISTEN),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await output.register_output(var, config)
    cg.add(var.set_port(config[CONF_PORT]))
