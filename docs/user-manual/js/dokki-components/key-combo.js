export const keyboardKey = {
    $tag: "keyboard-key",
    template: `
    <span class="keyboard-key">
        <slot/>
    </span>
    `,
}

export const keyCombo = {
    $tag: "key-combo",
    computed: {
        keys() {
            if (!this.$slots.default) {
                return [];
            }
            const keyString = this.$slots.default()[0].children;
            return keyString.split(" + ");
        }
    },
    template: `
    <span class="key-combo">
        <keyboard-key v-for="key in keys" :key="key">
            {{key}}
        </keyboard-key>
    </span>
    `,
};
